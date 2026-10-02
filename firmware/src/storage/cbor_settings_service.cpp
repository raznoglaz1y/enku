#include "enku/storage/cbor_settings_service.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>

namespace enku {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;
constexpr std::uint64_t kRecordTypeGlobalSettings = 1;

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
    } else if (value <= 0xFFFFFFFFULL) {
        out.push_back(static_cast<std::uint8_t>(prefix | 26U));
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    } else {
        out.push_back(static_cast<std::uint8_t>(prefix | 27U));
        for (int shift = 56; shift >= 0; shift -= 8) {
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

void appendBytes(
    std::vector<std::uint8_t>& out,
    const std::vector<std::uint8_t>& value
) {
    appendTypeValue(out, 2, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

void appendFloat32(
    std::vector<std::uint8_t>& out,
    float value
) {
    out.push_back(0xFA);

    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));

    out.push_back(static_cast<std::uint8_t>(raw >> 24U));
    out.push_back(static_cast<std::uint8_t>(raw >> 16U));
    out.push_back(static_cast<std::uint8_t>(raw >> 8U));
    out.push_back(static_cast<std::uint8_t>(raw));
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

    bool float32(float& value) {
        if (remaining() < 5U ||
            bytes_[offset_] != 0xFA) {
            return false;
        }

        ++offset_;
        std::uint32_t raw = 0;
        for (int i = 0; i < 4; ++i) {
            raw = (raw << 8U) | bytes_[offset_++];
        }

        std::memcpy(&value, &raw, sizeof(value));
        return std::isfinite(value);
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
    const GlobalSettings& settings
) {
    std::vector<std::uint8_t> out;
    appendArray(out, 10);

    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.locale)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.orientation)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.library_view)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.library_filter)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.library_sort)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.library_direction)
    );
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.reading_preset)
    );
    appendUnsigned(out, settings.font_size_px);
    appendFloat32(out, settings.line_spacing);
    appendArray(out, 2);
    appendUnsigned(out, settings.margin_px);
    appendUnsigned(
        out,
        static_cast<std::uint8_t>(settings.wifi_policy)
    );

    return out;
}

bool decodePayload(
    const std::vector<std::uint8_t>& payload,
    GlobalSettings& settings
) {
    Reader reader(payload);

    std::uint64_t count = 0;
    std::uint64_t locale = 0;
    std::uint64_t orientation = 0;
    std::uint64_t library_view = 0;
    std::uint64_t library_filter = 0;
    std::uint64_t library_sort = 0;
    std::uint64_t library_direction = 0;
    std::uint64_t reading_preset = 0;
    std::uint64_t font_size = 0;
    float line_spacing = 0.0F;
    std::uint64_t tail_count = 0;
    std::uint64_t margin = 0;
    std::uint64_t wifi_policy = 0;

    if (!reader.array(count) ||
        count != 10U ||
        !reader.unsignedValue(locale) ||
        !reader.unsignedValue(orientation) ||
        !reader.unsignedValue(library_view) ||
        !reader.unsignedValue(library_filter) ||
        !reader.unsignedValue(library_sort) ||
        !reader.unsignedValue(library_direction) ||
        !reader.unsignedValue(reading_preset) ||
        !reader.unsignedValue(font_size) ||
        !reader.float32(line_spacing) ||
        !reader.array(tail_count) ||
        tail_count != 2U ||
        !reader.unsignedValue(margin) ||
        !reader.unsignedValue(wifi_policy) ||
        !reader.finished()) {
        return false;
    }

    settings.locale = static_cast<LocaleId>(locale);
    settings.orientation =
        static_cast<Orientation>(orientation);
    settings.library_view =
        static_cast<LibraryView>(library_view);
    settings.library_filter =
        static_cast<LibraryFilter>(library_filter);
    settings.library_sort =
        static_cast<LibrarySort>(library_sort);
    settings.library_direction =
        static_cast<SortDirection>(library_direction);
    settings.reading_preset =
        static_cast<ReadingPreset>(reading_preset);
    settings.font_size_px =
        static_cast<std::uint16_t>(font_size);
    settings.line_spacing = line_spacing;
    settings.margin_px =
        static_cast<std::uint16_t>(margin);
    settings.wifi_policy =
        static_cast<WiFiPolicy>(wifi_policy);

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

CborSettingsService::CborSettingsService(
    StateFileStore& files
)
    : files_(files) {}

bool CborSettingsService::valid(
    const GlobalSettings& settings
) {
    return
        static_cast<std::uint8_t>(settings.locale) <=
            static_cast<std::uint8_t>(LocaleId::Ru) &&
        static_cast<std::uint8_t>(settings.orientation) <=
            static_cast<std::uint8_t>(Orientation::Landscape) &&
        static_cast<std::uint8_t>(settings.library_view) <=
            static_cast<std::uint8_t>(LibraryView::List) &&
        static_cast<std::uint8_t>(settings.library_filter) <=
            static_cast<std::uint8_t>(LibraryFilter::Finished) &&
        static_cast<std::uint8_t>(settings.library_sort) <=
            static_cast<std::uint8_t>(LibrarySort::RecentlyAdded) &&
        static_cast<std::uint8_t>(settings.library_direction) <=
            static_cast<std::uint8_t>(SortDirection::Descending) &&
        static_cast<std::uint8_t>(settings.reading_preset) <=
            static_cast<std::uint8_t>(ReadingPreset::Custom) &&
        settings.font_size_px >= 10U &&
        settings.font_size_px <= 72U &&
        std::isfinite(settings.line_spacing) &&
        settings.line_spacing >= 0.8F &&
        settings.line_spacing <= 2.5F &&
        settings.margin_px <= 160U &&
        static_cast<std::uint8_t>(settings.wifi_policy) <=
            static_cast<std::uint8_t>(
                WiFiPolicy::AutoConnectTrusted
            );
}

std::vector<std::uint8_t>
CborSettingsService::encode(
    std::uint32_t generation,
    const GlobalSettings& settings
) {
    const auto payload = encodePayload(settings);

    std::vector<std::uint8_t> out;
    appendArray(out, 5);
    appendUnsigned(out, kSchemaVersion);
    appendUnsigned(out, kRecordTypeGlobalSettings);
    appendUnsigned(out, generation);
    appendBytes(out, payload);
    appendUnsigned(out, crc32(payload));
    return out;
}

bool CborSettingsService::decode(
    const std::vector<std::uint8_t>& bytes,
    DecodedSettings& decoded
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
        type != kRecordTypeGlobalSettings ||
        generation >
            std::numeric_limits<std::uint32_t>::max() ||
        stored_crc != crc32(payload)) {
        return false;
    }

    GlobalSettings settings;
    if (!decodePayload(payload, settings) ||
        !valid(settings)) {
        return false;
    }

    decoded.generation =
        static_cast<std::uint32_t>(generation);
    decoded.settings = settings;
    return true;
}

PersistStatus CborSettingsService::readSlot(
    const std::string& path,
    DecodedSettings& decoded
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

PersistStatus CborSettingsService::load(
    GlobalSettings& settings
) {
    DecodedSettings a;
    DecodedSettings b;

    const auto a_status = readSlot(kSlotA, a);
    const auto b_status = readSlot(kSlotB, b);

    const bool a_ok = a_status == PersistStatus::Ok;
    const bool b_ok = b_status == PersistStatus::Ok;

    if (a_ok || b_ok) {
        settings =
            a_ok && (!b_ok || a.generation >= b.generation)
                ? a.settings
                : b.settings;
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

PersistStatus CborSettingsService::save(
    const GlobalSettings& settings
) {
    if (!valid(settings)) {
        return PersistStatus::InvalidRecord;
    }

    DecodedSettings a;
    DecodedSettings b;

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
            encode(max_generation + 1U, settings)
        )
    );
}

} // namespace enku
