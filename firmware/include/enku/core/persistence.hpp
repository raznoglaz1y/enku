#pragma once

#include <cstdint>

namespace enku {

enum class PersistRecordType : std::uint8_t {
    LibraryIndex,
    GlobalSettings,
    BookState,
    AppContext,
    BootMarker,
};

struct PersistRecordHeader {
    std::uint16_t schema_version{1};
    PersistRecordType record_type{PersistRecordType::LibraryIndex};
    std::uint32_t generation{0};
    std::uint32_t payload_length{0};
    std::uint32_t payload_crc32{0};
};

enum class PersistStatus : std::uint8_t {
    Ok,
    NotFound,
    InvalidRecord,
    UnsupportedSchema,
    IoError,
    NoSpace,
};

} // namespace enku
