#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace enku {

enum class ZipArchiveStatus : std::uint8_t {
    Ok,
    InvalidArchive,
    EntryNotFound,
    UnsupportedCompression,
    DecompressionFailed,
    EntryTooLarge,
};

struct ZipEntry {
    std::string name;
    std::uint16_t compression_method{0};
    std::uint32_t compressed_size{0};
    std::uint32_t uncompressed_size{0};
    std::uint32_t local_header_offset{0};
};

class ZipArchive {
public:
    static constexpr std::size_t kMaxEntryBytes =
        16U * 1024U * 1024U;

    explicit ZipArchive(
        std::string_view bytes
    );

    ZipArchiveStatus status() const;
    const std::vector<ZipEntry>& entries() const;

    std::optional<ZipEntry> find(
        std::string_view name
    ) const;

    ZipArchiveStatus read(
        std::string_view name,
        std::string& out
    ) const;

private:
    std::string_view bytes_;
    ZipArchiveStatus status_{
        ZipArchiveStatus::InvalidArchive
    };
    std::vector<ZipEntry> entries_;

    bool parseCentralDirectory();
};

} // namespace enku
