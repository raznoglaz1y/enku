#include "enku/storage/cbor_app_context_service.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 2;
constexpr std::uint64_t kLegacySchemaVersion = 1;
constexpr std::uint64_t kRecordTypeAppContext = 3;

void appendTypeValue(
    std::vector<std::uint8_t>& out,
    std::uint8_t major,
    std::uint64_t value
) {
    const auto prefix = static_cast<std::uint8_t>(major << 5U);

    if (value < 24U) {
        out.push_back(static_cast<std::uint8_t>(prefix | value));
    } else if (value <= 0xFFU) {
        out.push_back(static_cast<std::uint8_t>(prefix | 24U));
        out.push_back(static_cast<std::uint8_t>(value));
    } else if (value <= 0xFFFFU) {
        out.push_back(static_cast<std::uint8_t>(prefix | 25U));
        out.push_back(static_cast<std::uint8_t>(value >> 8U));
        out.push_back(static_cast<std::uint8_t>(value));
    } else {
        out.push_back(static_cast<std::uint8_t>(prefix | 26U));
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }
}

void appendUnsigned(
    std::vector<std::uint8_t>& out,
    std::uint64_t value
) {
    appendTypeValue(out, 0, value);
}

void appendArray(
    std::vector<std::uint8_t>& out,
    std::uint64_t count
) {
    appendTypeValue(out, 4, count);
}

void appendText(
    std::vector<std::uint8_t>& out,
    const std::string& value
) {
    appendTypeValue(out, 3, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void appendNull(std::vector<std::uint8_t>& out) {
    out.push_back(0xF6);
}

void appendBytes(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& value
) {
    appendTypeValue(out, 2, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

std::uint32_t crc32(
    const std::vector<std::uint8_t>& bytes
) {
    std::uint32_t crc = 0xFFFFFFFFU;

    for (const auto byte : bytes) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const auto mask =
                static_cast<std::uint32_t>(
                    -(static_cast<std::int32_t>(crc & 1U))
                );
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }

    return ~crc;
}

class Reader {
public:
    explicit Reader(
        const std::vector<std::uint8_t>& bytes
    )
        : bytes_(bytes) {}

    bool array(std::uint64_t& count) {
        return typedValue(4, count);
    }

    bool unsignedValue(std::uint64_t& value) {
        return typedValue(0, value);
    }

    bool text(std::string& value) {
        std::uint64_t length = 0;
        if (!typedValue(3, length) ||
            length > remaining()) {
            return false;
        }

        value.assign(
            reinterpret_cast<const char*>(
                bytes_.data() + offset_
            ),
            static_cast<std::size_t>(length)
        );
        offset_ += static_cast<std::size_t>(length);
        return true;
    }

    bool optionalText(
        std::optional<std::string>& value
    ) {
        if (remaining() > 0 &&
            bytes_[offset_] == 0xF6) {
            ++offset_;
            value.reset();
            return true;
        }

        std::string decoded;
        if (!text(decoded)) {
            return false;
        }

        value = std::move(decoded);
        return true;
    }

    bool byteString(
        std::vector<std::uint8_t>& value
    ) {
        std::uint64_t length = 0;
        if (!typedValue(2, length) ||
            length > remaining()) {
            return false;
        }

        const auto begin =
            bytes_.begin() +
            static_cast<std::ptrdiff_t>(offset_);
        const auto end =
            begin +
            static_cast<std::ptrdiff_t>(length);

        value.assign(begin, end);
        offset_ += static_cast<std::size_t>(length);
        return true;
    }

    bool finished() const {
        return offset_ == bytes_.size();
    }

private:
    const std::vector<std::uint8_t>& bytes_;
    std::size_t offset_{0};

    std::size_t remaining() const {
        return bytes_.size() - offset_;
    }

    bool typedValue(
        std::uint8_t expected_major,
        std::uint64_t& value
    ) {
        if (remaining() == 0) {
            return false;
        }

        const auto initial = bytes_[offset_++];
        const auto major =
            static_cast<std::uint8_t>(initial >> 5U);
        const auto additional =
            static_cast<std::uint8_t>(initial & 0x1FU);

        if (major != expected_major) {
            return false;
        }

        if (additional < 24U) {
            value = additional;
            return true;
        }

        std::size_t width = 0;
        switch (additional) {
            case 24: width = 1; break;
            case 25: width = 2; break;
            case 26: width = 4; break;
            case 27: width = 8; break;
            default: return false;
        }

        if (remaining() < width) {
            return false;
        }

        value = 0;
        for (std::size_t i = 0; i < width; ++i) {
            value = (value << 8U) | bytes_[offset_++];
        }

        return true;
    }
};

std::vector<std::uint8_t> encodePayload(
    const AppRestoreContext& context
) {
    std::vector<std::uint8_t> out;
    appendArray(out, 4);
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(context.screen)
    );

    if (context.current_book.has_value()) {
        appendText(out, *context.current_book);
    } else {
        appendNull(out);
    }

    appendUnsigned(
        out,
        context.library_offset
    );

    if (context.library_focused_book.has_value()) {
        appendText(
            out,
            *context.library_focused_book
        );
    } else {
        appendNull(out);
    }

    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    AppRestoreContext& context
) {
    Reader reader(payload);
    std::uint64_t count = 0;
    std::uint64_t screen = 0;

    if (!reader.array(count) ||
        (count != 2U && count != 4U) ||
        !reader.unsignedValue(screen) ||
        screen > static_cast<std::uint8_t>(
            Screen::ErrorRecovery
        ) ||
        !reader.optionalText(context.current_book)) {
        return false;
    }

    context.library_offset = 0;
    context.library_focused_book.reset();

    if (count == 4U) {
        std::uint64_t library_offset = 0;

        if (!reader.unsignedValue(library_offset) ||
            library_offset >
                std::numeric_limits<std::uint32_t>::max() ||
            !reader.optionalText(
                context.library_focused_book
            )) {
            return false;
        }

        context.library_offset =
            static_cast<std::uint32_t>(
                library_offset
            );
    }

    if (!reader.finished()) {
        return false;
    }

    context.screen = static_cast<Screen>(screen);

    if (context.screen != Screen::Library &&
        context.screen != Screen::Reading) {
        return false;
    }

    if (context.screen == Screen::Reading &&
        (!context.current_book.has_value() ||
         context.current_book->empty())) {
        return false;
    }

    if (context.screen == Screen::Library) {
        context.current_book.reset();
    }

    return true;
}

PersistStatus mapFileStatus(StateFileStatus status) {
    switch (status) {
        case StateFileStatus::Ok:
            return PersistStatus::Ok;
        case StateFileStatus::NotFound:
            return PersistStatus::NotFound;
        case StateFileStatus::NoSpace:
            return PersistStatus::NoSpace;
        case StateFileStatus::IoError:
        default:
            return PersistStatus::IoError;
    }
}

} // namespace

CborAppContextService::CborAppContextService(
    StateFileStore& files
)
    : files_(files) {}

std::vector<std::uint8_t>
CborAppContextService::encode(
    std::uint32_t generation,
    const AppRestoreContext& context
) {
    const auto payload = encodePayload(context);

    std::vector<std::uint8_t> out;
    appendArray(out, 5);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeAppContext);
    appendUnsigned(out, generation);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));
    return out;
}

bool CborAppContextService::decode(
    const std::vector<std::uint8_t>& bytes,
    DecodedContext& decoded
) {
    Reader reader(bytes);

    std::uint64_t count = 0;
    std::uint64_t schema = 0;
    std::uint64_t type = 0;
    std::uint64_t generation = 0;
    std::vector<std::uint8_t> payload;
    std::uint64_t stored_crc = 0;

    if (!reader.array(count) ||
        count != 5U ||
        !reader.unsignedValue(schema) ||
        !reader.unsignedValue(type) ||
        !reader.unsignedValue(generation) ||
        !reader.byteString(payload) ||
        !reader.unsignedValue(stored_crc) ||
        !reader.finished()) {
        return false;
    }

    if ((schema != kSchemaVersion &&
         schema != kLegacySchemaVersion) ||
        type != kRecordTypeAppContext ||
        generation >
            std::numeric_limits<std::uint32_t>::max() ||
        stored_crc != crc32(payload)) {
        return false;
    }

    AppRestoreContext context;
    if (!decodePayload(payload, context)) {
        return false;
    }

    decoded.generation =
        static_cast<std::uint32_t>(generation);
    decoded.context = std::move(context);
    return true;
}

PersistStatus CborAppContextService::readSlot(
    const std::string& path,
    DecodedContext& decoded
) {
    std::vector<std::uint8_t> bytes;
    const auto status = files_.read(path, bytes);

    if (status != StateFileStatus::Ok) {
        return mapFileStatus(status);
    }

    if (!decode(bytes, decoded)) {
        return PersistStatus::InvalidRecord;
    }

    return PersistStatus::Ok;
}

PersistStatus CborAppContextService::load(
    AppRestoreContext& context
) {
    DecodedContext a;
    DecodedContext b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    const bool a_ok = a_status == PersistStatus::Ok;
    const bool b_ok = b_status == PersistStatus::Ok;

    if (a_ok || b_ok) {
        context =
            a_ok && (!b_ok || a.generation >= b.generation)
                ? a.context
                : b.context;
        return PersistStatus::Ok;
    }

    if (a_status == PersistStatus::NotFound &&
        b_status == PersistStatus::NotFound) {
        return PersistStatus::NotFound;
    }

    if (a_status == PersistStatus::IoError ||
        b_status == PersistStatus::IoError) {
        return PersistStatus::IoError;
    }

    return PersistStatus::InvalidRecord;
}

PersistStatus CborAppContextService::save(
    const AppRestoreContext& context
) {
    if (context.screen != Screen::Library &&
        context.screen != Screen::Reading) {
        return PersistStatus::InvalidRecord;
    }

    if (context.screen == Screen::Reading &&
        (!context.current_book.has_value() ||
         context.current_book->empty())) {
        return PersistStatus::InvalidRecord;
    }

    DecodedContext a;
    DecodedContext b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    if (a_status == PersistStatus::IoError ||
        b_status == PersistStatus::IoError) {
        return PersistStatus::IoError;
    }

    const bool a_ok = a_status == PersistStatus::Ok;
    const bool b_ok = b_status == PersistStatus::Ok;

    const auto max_generation = std::max(
        a_ok ? a.generation : 0U,
        b_ok ? b.generation : 0U
    );

    if (max_generation ==
        std::numeric_limits<std::uint32_t>::max()) {
        return PersistStatus::InvalidRecord;
    }

    const char* target = kSlotA;

    if (!a_ok) {
        target = kSlotA;
    } else if (!b_ok) {
        target = kSlotB;
    } else {
        target =
            a.generation <= b.generation
                ? kSlotA
                : kSlotB;
    }

    return mapFileStatus(
        files_.write(
            target,
            encode(max_generation + 1U, context)
        )
    );
}

} // namespace enku
