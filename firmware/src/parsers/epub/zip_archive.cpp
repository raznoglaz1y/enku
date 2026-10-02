#include "enku/reader/zip_archive.hpp"

#include <algorithm>
#include <cstring>

#include <zlib.h>

namespace enku {
namespace {

std::uint16_t readU16(
    std::string_view bytes,
    std::size_t offset
) {
    return static_cast<std::uint16_t>(
        static_cast<unsigned char>(bytes[offset]) |
        (static_cast<std::uint16_t>(
            static_cast<unsigned char>(
                bytes[offset + 1]
            )
        ) << 8U)
    );
}

std::uint32_t readU32(
    std::string_view bytes,
    std::size_t offset
) {
    return
        static_cast<std::uint32_t>(
            static_cast<unsigned char>(
                bytes[offset]
            )
        ) |
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(
                bytes[offset + 1]
            )
        ) << 8U) |
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(
                bytes[offset + 2]
            )
        ) << 16U) |
        (static_cast<std::uint32_t>(
            static_cast<unsigned char>(
                bytes[offset + 3]
            )
        ) << 24U);
}

bool hasRange(
    std::string_view bytes,
    std::size_t offset,
    std::size_t length
) {
    return offset <= bytes.size() &&
        length <= bytes.size() - offset;
}

} // namespace

ZipArchive::ZipArchive(
    std::string_view bytes
)
    : bytes_(bytes) {
    status_ = parseCentralDirectory()
        ? ZipArchiveStatus::Ok
        : ZipArchiveStatus::InvalidArchive;
}

ZipArchiveStatus ZipArchive::status() const {
    return status_;
}

const std::vector<ZipEntry>&
ZipArchive::entries() const {
    return entries_;
}

bool ZipArchive::parseCentralDirectory() {
    constexpr std::uint32_t kEocdSignature =
        0x06054B50U;
    constexpr std::uint32_t kCentralSignature =
        0x02014B50U;

    if (bytes_.size() < 22U) {
        return false;
    }

    const std::size_t search_start =
        bytes_.size() > 65557U
            ? bytes_.size() - 65557U
            : 0U;

    std::optional<std::size_t> eocd_offset;

    for (std::size_t pos = bytes_.size() - 22U;;
         --pos) {
        if (readU32(bytes_, pos) ==
            kEocdSignature) {
            eocd_offset = pos;
            break;
        }

        if (pos == search_start) {
            break;
        }
    }

    if (!eocd_offset.has_value()) {
        return false;
    }

    const auto eocd = *eocd_offset;

    if (!hasRange(bytes_, eocd, 22U)) {
        return false;
    }

    const auto entry_count =
        readU16(bytes_, eocd + 10U);
    const auto central_size =
        readU32(bytes_, eocd + 12U);
    const auto central_offset =
        readU32(bytes_, eocd + 16U);

    if (!hasRange(
            bytes_,
            central_offset,
            central_size
        )) {
        return false;
    }

    entries_.clear();
    entries_.reserve(entry_count);

    std::size_t cursor = central_offset;

    for (std::uint16_t index = 0;
         index < entry_count;
         ++index) {
        if (!hasRange(bytes_, cursor, 46U) ||
            readU32(bytes_, cursor) !=
                kCentralSignature) {
            entries_.clear();
            return false;
        }

        const auto method =
            readU16(bytes_, cursor + 10U);
        const auto compressed_size =
            readU32(bytes_, cursor + 20U);
        const auto uncompressed_size =
            readU32(bytes_, cursor + 24U);
        const auto name_length =
            readU16(bytes_, cursor + 28U);
        const auto extra_length =
            readU16(bytes_, cursor + 30U);
        const auto comment_length =
            readU16(bytes_, cursor + 32U);
        const auto local_offset =
            readU32(bytes_, cursor + 42U);

        const std::size_t record_size =
            46U +
            static_cast<std::size_t>(
                name_length
            ) +
            static_cast<std::size_t>(
                extra_length
            ) +
            static_cast<std::size_t>(
                comment_length
            );

        if (!hasRange(
                bytes_,
                cursor,
                record_size
            )) {
            entries_.clear();
            return false;
        }

        ZipEntry entry;
        entry.name.assign(
            bytes_.substr(
                cursor + 46U,
                name_length
            )
        );
        entry.compression_method = method;
        entry.compressed_size = compressed_size;
        entry.uncompressed_size = uncompressed_size;
        entry.local_header_offset = local_offset;
        entries_.push_back(std::move(entry));

        cursor += record_size;
    }

    return true;
}

std::optional<ZipEntry> ZipArchive::find(
    std::string_view name
) const {
    const auto it = std::find_if(
        entries_.begin(),
        entries_.end(),
        [&](const ZipEntry& entry) {
            return entry.name == name;
        }
    );

    if (it == entries_.end()) {
        return std::nullopt;
    }

    return *it;
}

ZipArchiveStatus ZipArchive::read(
    std::string_view name,
    std::string& out
) const {
    constexpr std::uint32_t kLocalSignature =
        0x04034B50U;

    const auto found = find(name);
    if (!found.has_value()) {
        return ZipArchiveStatus::EntryNotFound;
    }

    const auto& entry = *found;

    if (entry.uncompressed_size >
        kMaxEntryBytes) {
        return ZipArchiveStatus::EntryTooLarge;
    }

    const std::size_t local =
        entry.local_header_offset;

    if (!hasRange(bytes_, local, 30U) ||
        readU32(bytes_, local) !=
            kLocalSignature) {
        return ZipArchiveStatus::InvalidArchive;
    }

    const auto name_length =
        readU16(bytes_, local + 26U);
    const auto extra_length =
        readU16(bytes_, local + 28U);

    const std::size_t data_offset =
        local + 30U +
        static_cast<std::size_t>(
            name_length
        ) +
        static_cast<std::size_t>(
            extra_length
        );

    if (!hasRange(
            bytes_,
            data_offset,
            entry.compressed_size
        )) {
        return ZipArchiveStatus::InvalidArchive;
    }

    const auto compressed =
        bytes_.substr(
            data_offset,
            entry.compressed_size
        );

    if (entry.compression_method == 0U) {
        out.assign(compressed);
        return ZipArchiveStatus::Ok;
    }

    if (entry.compression_method != 8U) {
        return ZipArchiveStatus::UnsupportedCompression;
    }

    out.assign(
        entry.uncompressed_size,
        '\0'
    );

    z_stream stream = {};
    stream.next_in =
        reinterpret_cast<Bytef*>(
            const_cast<char*>(
                compressed.data()
            )
        );
    stream.avail_in =
        static_cast<uInt>(
            compressed.size()
        );
    stream.next_out =
        reinterpret_cast<Bytef*>(
            out.data()
        );
    stream.avail_out =
        static_cast<uInt>(
            out.size()
        );

    if (inflateInit2(
            &stream,
            -MAX_WBITS
        ) != Z_OK) {
        out.clear();
        return ZipArchiveStatus::DecompressionFailed;
    }

    const auto result =
        inflate(
            &stream,
            Z_FINISH
        );

    inflateEnd(&stream);

    if (result != Z_STREAM_END ||
        stream.total_out !=
            entry.uncompressed_size) {
        out.clear();
        return ZipArchiveStatus::DecompressionFailed;
    }

    return ZipArchiveStatus::Ok;
}

} // namespace enku
