#include "enku/reader/parser_memory_budget.hpp"
#include "enku/reader/zip_archive.hpp"

#include <cassert>

using namespace enku;

int main() {
    static_assert(
        ParserMemoryBudget::kRangeChunkBytes ==
        32U * 1024U
    );
    static_assert(
        ParserMemoryBudget::kFb2MetadataBytes ==
        512U * 1024U
    );
    static_assert(
        ParserMemoryBudget::kTextResourceBytes ==
        4U * 1024U * 1024U
    );
    static_assert(
        ParserMemoryBudget::kZipCentralDirectoryBytes ==
        2U * 1024U * 1024U
    );
    static_assert(
        ParserMemoryBudget::kZipMaxEntries ==
        4096U
    );
    static_assert(
        ParserMemoryBudget::kZipEocdWindowBytes ==
        65557U
    );

    static_assert(
        ParserMemoryBudget::kRangeChunkBytes <
        ParserMemoryBudget::kFb2MetadataBytes
    );
    static_assert(
        ParserMemoryBudget::kFb2MetadataBytes <
        ParserMemoryBudget::kTextResourceBytes
    );

    assert(
        ZipArchive::kMaxEntryBytes ==
        ParserMemoryBudget::kTextResourceBytes
    );
    assert(
        ZipArchive::kMaxCentralDirectoryBytes ==
        ParserMemoryBudget::kZipCentralDirectoryBytes
    );
    assert(
        ZipArchive::kMaxEntries ==
        ParserMemoryBudget::kZipMaxEntries
    );

    return 0;
}
