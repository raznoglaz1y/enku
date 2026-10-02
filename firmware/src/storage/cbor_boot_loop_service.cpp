#include "enku/storage/cbor_boot_loop_service.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;
constexpr std::uint64_t kRecordTypeBootMarker = 4;

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
            out.push_back(
                static_cast<std::uint8_t>(value >> shift)
            );
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

void appendBytes(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& value
) {
    appendTypeValue(out, 2, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void appendBool(
    std::vector<std::uint8_t>& out,
    bool value
) {
    out.push_back(value ? 0xF5 : 0xF4);
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

    bool boolean(bool& value) {
        if (remaining() == 0) {
            return false;
        }

        if (bytes_[offset_] == 0xF4) {
            value = false;
            ++offset_;
            return true;
        }

        if (bytes_[offset_] == 0xF5) {
            value = true;
            ++offset_;
            return true;
        }

        return false;
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
    const BootLoopMarker& marker
) {
    std::vector<std::uint8_t> out;
    appendArray(out, 2);
    appendUnsigned(out, marker.incomplete_boot_count);
    appendBool(out, marker.stable);
    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    BootLoopMarker& marker
) {
    Reader reader(payload);

    std::uint64_t count = 0;
    std::uint64_t incomplete = 0;
    bool stable = false;

    if (!reader.array(count) ||
        count != 2U ||
        !reader.unsignedValue(incomplete) ||
        incomplete > 255U ||
        !reader.boolean(stable) ||
        !reader.finished()) {
        return false;
    }

    marker.incomplete_boot_count =
        static_cast<std::uint8_t>(incomplete);
    marker.stable = stable;
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

CborBootLoopService::CborBootLoopService(
    StateFileStore& files
)
    : files_(files) {}

std::vector<std::uint8_t>
CborBootLoopService::encode(
    std::uint32_t generation,
    const BootLoopMarker& marker
) {
    const auto payload = encodePayload(marker);

    std::vector<std::uint8_t> out;
    appendArray(out, 5);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeBootMarker);
    appendUnsigned(out, generation);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));
    return out;
}

bool CborBootLoopService::decode(
    const std::vector<std::uint8_t>& bytes,
    DecodedMarker& decoded
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

    if (schema != kSchemaVersion ||
        type != kRecordTypeBootMarker ||
        generation >
            std::numeric_limits<std::uint32_t>::max() ||
        stored_crc != crc32(payload)) {
        return false;
    }

    BootLoopMarker marker;
    if (!decodePayload(payload, marker)) {
        return false;
    }

    decoded.generation =
        static_cast<std::uint32_t>(generation);
    decoded.marker = marker;
    return true;
}

PersistStatus CborBootLoopService::readSlot(
    const std::string& path,
    DecodedMarker& decoded
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

PersistStatus CborBootLoopService::load(
    BootLoopMarker& marker
) {
    DecodedMarker a;
    DecodedMarker b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    const bool a_ok = a_status == PersistStatus::Ok;
    const bool b_ok = b_status == PersistStatus::Ok;

    if (a_ok || b_ok) {
        marker =
            a_ok && (!b_ok || a.generation >= b.generation)
                ? a.marker
                : b.marker;
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

PersistStatus CborBootLoopService::save(
    const BootLoopMarker& marker
) {
    DecodedMarker a;
    DecodedMarker b;

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
            encode(max_generation + 1U, marker)
        )
    );
}

PersistStatus CborBootLoopService::beginBoot(
    BootLoopMarker& marker
) {
    BootLoopMarker previous;
    const auto status = load(previous);

    if (status == PersistStatus::NotFound) {
        previous = BootLoopMarker{};
    } else if (status != PersistStatus::Ok) {
        return status;
    }

    const std::uint16_t next =
        previous.stable
            ? 1U
            : static_cast<std::uint16_t>(
                previous.incomplete_boot_count
            ) + 1U;

    marker.incomplete_boot_count =
        static_cast<std::uint8_t>(
            std::min<std::uint16_t>(next, 255U)
        );
    marker.stable = false;

    return save(marker);
}

PersistStatus CborBootLoopService::markStable() {
    return save(
        BootLoopMarker{
            0,
            true,
        }
    );
}

} // namespace enku
