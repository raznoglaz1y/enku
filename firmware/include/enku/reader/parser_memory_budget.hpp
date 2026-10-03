#pragma once

#include <cstddef>
#include <cstdint>

namespace enku {

struct ParserMemoryBudget {
    // Shared streaming/ranged-read working chunk. Keeps transient input
    // buffers small enough for constrained ESP32-S3 operation.
    static constexpr std::size_t kRangeChunkBytes =
        32U * 1024U;

    // Metadata-only FB2 parsing stops growing its prefix buffer here.
    static constexpr std::size_t kFb2MetadataBytes =
        512U * 1024U;

    // Upper bound for one textual resource materialized at once.
    // Used by EPUB ZIP entries and FB2 leaf sections.
    static constexpr std::size_t kTextResourceBytes =
        4U * 1024U * 1024U;

    // ZIP directory bounds prevent hostile/accidental archive metadata from
    // consuming desktop-class RAM before any book content is read.
    static constexpr std::size_t kZipCentralDirectoryBytes =
        2U * 1024U * 1024U;

    static constexpr std::uint16_t kZipMaxEntries =
        4096U;

    // ZIP EOCD may sit behind a maximum 65535-byte comment plus the
    // 22-byte EOCD record itself.
    static constexpr std::uint64_t kZipEocdWindowBytes =
        65557U;
};

} // namespace enku
