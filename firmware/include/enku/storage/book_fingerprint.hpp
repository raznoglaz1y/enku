#pragma once

#include <cstdint>
#include <string>

#include "book_file_store.hpp"

namespace enku {

struct BookFingerprintResult {
    BookFileStatus status{BookFileStatus::IoError};
    std::uint64_t file_size{0};
    std::string fingerprint;

    bool ok() const {
        return status == BookFileStatus::Ok;
    }
};

std::string fingerprintBookBytes(
    const std::string& bytes
);

BookFingerprintResult fingerprintStoredBook(
    BookFileStore& files,
    const std::string& source_path
);

} // namespace enku
