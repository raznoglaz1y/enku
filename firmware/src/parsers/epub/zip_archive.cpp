#include "enku/reader/zip_archive.hpp"

#include <algorithm>
#include <limits>

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

ZipArchive::ZipArchive(
    const ZipRangeSource& source
)
    : source_(&source) {
    status_ = parseCentralDirectory()
        ? ZipArchiveStatus::Ok
        : ZipArchiveStatus::InvalidArchive;
}

std::uint64_t ZipArchive::sourceSize() const {
    return source_ != nullptr
        ? source_->size()
        : static_cast<std::uint64_t>(
              bytes_.size()
          );
}

bool ZipArchive::readRange(
    std::uint64_t offset,
    std::size_t length,
    std::string& out
) const {
    const auto total = sourceSize();

    if (offset > total ||
        static_cast<std::uint64_t>(length) >
            total - offset) {
        out.clear();
        return false;
    }

    if (source_ != nullptr) {
        return source_->readRange(
            offset,
            length,
            out
        );
    }

    if (offset >
        static_cast<std::uint64_t>(
            std::numeric_limits<
                std::size_t
            >::max()
        )) {
        out.clear();
        return false;
    }

    out.assign(
        bytes_.substr(
            static_cast<std::size_t>(
                offset
            ),
            length
        )
    );
    return true;
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
    constexpr std::uint64_t kMaxEocdWindow =
        65557U;

    const auto total = sourceSize();

    if (total < 22U) {
        return false;
    }

    const auto tail_size =
        static_cast<std::size_t>(
            std::min<std::uint64_t>(
                total,
                kMaxEocdWindow
            )
        );

    const auto tail_offset =
        total - tail_size;

    std::string tail;

    if (!readRange(
            tail_offset,
            tail_size,
            tail
        )) {
        return false;
    }

    std::optional<std::size_t> eocd_offset;

    for (std::size_t pos =
             tail.size() - 22U;;
         --pos) {
        if (readU32(tail, pos) ==
            kEocdSignature) {
            eocd_offset = pos;
            break;
        }

        if (pos == 0U) {
            break;
        }
    }

    if (!eocd_offset.has_value() ||
        !hasRange(
            tail,
            *eocd_offset,
            22U
        )) {
        return false;
    }

    const auto eocd =
        *eocd_offset;

    const auto entry_count =
        readU16(tail, eocd + 10U);
    const auto central_size =
        readU32(tail, eocd + 12U);
    const auto central_offset =
        readU32(tail, eocd + 16U);

    if (static_cast<std::uint64_t>(
            central_offset
        ) > total ||
        static_cast<std::uint64_t>(
            central_size
        ) >
            total -
                static_cast<std::uint64_t>(
                    central_offset
                )) {
        return false;
    }

    std::string central;

    if (!readRange(
            central_offset,
            central_size,
            central
        )) {
        return false;
    }

    entries_.clear();
    entries_.reserve(entry_count);

    std::size_t cursor = 0;

    for (std::uint16_t index = 0;
         index < entry_count;
         ++index) {
        if (!hasRange(
                central,
                cursor,
                46U
            ) ||
            readU32(
                central,
                cursor
            ) !=
                kCentralSignature) {
            entries_.clear();
            return false;
        }

        const auto method =
            readU16(
                central,
                cursor + 10U
            );
        const auto compressed_size =
            readU32(
                central,
                cursor + 20U
            );
        const auto uncompressed_size =
            readU32(
                central,
                cursor + 24U
            );
        const auto name_length =
            readU16(
                central,
                cursor + 28U
            );
        const auto extra_length =
            readU16(
                central,
                cursor + 30U
            );
        const auto comment_length =
            readU16(
                central,
                cursor + 32U
            );
        const auto local_offset =
            readU32(
                central,
                cursor + 42U
            );

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
                central,
                cursor,
                record_size
            )) {
            entries_.clear();
            return false;
        }

        ZipEntry entry;
        entry.name.assign(
            central.substr(
                cursor + 46U,
                name_length
            )
        );
        entry.compression_method = method;
        entry.compressed_size =
            compressed_size;
        entry.uncompressed_size =
            uncompressed_size;
        entry.local_header_offset =
            local_offset;

        entries_.push_back(
            std::move(entry)
        );

        cursor += record_size;
    }

    return true;
}

std::optional<ZipEntry> ZipArchive::find(
    std::string_view name
) const {
    const auto it =
        std::find_if(
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

    std::string header;

    if (!readRange(
            entry.local_header_offset,
            30U,
            header
        ) ||
        readU32(header, 0U) !=
            kLocalSignature) {
        return ZipArchiveStatus::InvalidArchive;
    }

    const auto name_length =
        readU16(header, 26U);
    const auto extra_length =
        readU16(header, 28U);

    const auto data_offset =
        static_cast<std::uint64_t>(
            entry.local_header_offset
        ) +
        30U +
        static_cast<std::uint64_t>(
            name_length
        ) +
        static_cast<std::uint64_t>(
            extra_length
        );

    std::string compressed;

    if (!readRange(
            data_offset,
            entry.compressed_size,
            compressed
        )) {
        return ZipArchiveStatus::InvalidArchive;
    }

    if (entry.compression_method == 0U) {
        out = std::move(compressed);
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
            compressed.data()
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
