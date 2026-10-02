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

class ZipRangeSource {
public:
    virtual ~ZipRangeSource() = default;

    virtual std::uint64_t size() const = 0;

    virtual bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const = 0;
};

class ZipArchive {
public:
    static constexpr std::size_t kMaxEntryBytes =
        16U * 1024U * 1024U;

    explicit ZipArchive(
        std::string_view bytes
    );

    explicit ZipArchive(
        const ZipRangeSource& source
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
    const ZipRangeSource* source_{nullptr};
    ZipArchiveStatus status_{
        ZipArchiveStatus::InvalidArchive
    };
    std::vector<ZipEntry> entries_;

    std::uint64_t sourceSize() const;

    bool readRange(
        std::uint64_t offset,
        std::size_t length,
        std::string& out
    ) const;

    bool parseCentralDirectory();
};

} // namespace enku
