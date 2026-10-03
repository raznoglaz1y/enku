#include "enku/storage/book_fingerprint.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace enku {
namespace {

constexpr std::uint64_t kFnvOffset =
    14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime =
    1099511628211ULL;
constexpr std::size_t kFingerprintChunkBytes =
    64U * 1024U;

void updateFingerprint(
    std::uint64_t& hash,
    std::string_view bytes
) {
    for (const unsigned char ch : bytes) {
        hash ^= ch;
        hash *= kFnvPrime;
    }
}

std::string finishFingerprint(
    std::uint64_t hash,
    std::uint64_t size
) {
    std::ostringstream out;
    out << "fnv1a64:"
        << std::hex
        << std::setfill('0')
        << std::setw(16)
        << hash
        << ":"
        << std::dec
        << size;
    return out.str();
}

} // namespace

std::string fingerprintBookBytes(
    const std::string& bytes
) {
    std::uint64_t hash = kFnvOffset;
    updateFingerprint(hash, bytes);
    return finishFingerprint(
        hash,
        static_cast<std::uint64_t>(bytes.size())
    );
}

BookFingerprintResult fingerprintStoredBook(
    BookFileStore& files,
    const std::string& source_path,
    std::uint64_t known_size
) {
    BookFingerprintResult result;

    if (known_size == UINT64_MAX) {
        const auto size_status =
            files.size(source_path, result.file_size);
        if (size_status != BookFileStatus::Ok) {
            result.status = size_status;
            return result;
        }
    } else {
        result.file_size = known_size;
    }

    std::uint64_t hash = kFnvOffset;
    std::uint64_t offset = 0;

    while (offset < result.file_size) {
        const auto remaining =
            result.file_size - offset;
        const auto chunk_size =
            static_cast<std::size_t>(
                std::min<std::uint64_t>(
                    remaining,
                    kFingerprintChunkBytes
                )
            );

        std::string chunk;
        const auto read_status =
            files.readRange(
                source_path,
                offset,
                chunk_size,
                chunk
            );
        if (read_status != BookFileStatus::Ok ||
            chunk.size() != chunk_size) {
            result.status =
                read_status == BookFileStatus::Ok
                    ? BookFileStatus::IoError
                    : read_status;
            return result;
        }

        updateFingerprint(hash, chunk);
        offset += chunk_size;
    }

    result.fingerprint =
        finishFingerprint(hash, result.file_size);
    result.status = BookFileStatus::Ok;
    return result;
}

} // namespace enku
