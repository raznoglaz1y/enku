#pragma once

#include <cstdint>
#include <string>

#include "../services/services.hpp"
#include "../reader/txt_parser.hpp"
#include "../reader/epub_parser.hpp"

namespace enku {

enum class BookImportStatus : std::uint8_t {
    Ok,
    EmptySource,
    UnsupportedFormat,
    ParseFailed,
    Duplicate,
    LibraryCommitFailed,
};

struct BookImportSource {
    std::string source_path;
    std::string source_filename;
    std::string bytes;
};

struct PreparedBookImport {
    BookImportStatus status{BookImportStatus::ParseFailed};
    BookRecord record;

    bool ok() const {
        return status == BookImportStatus::Ok;
    }
};

struct BookImportResult {
    BookImportStatus status{BookImportStatus::ParseFailed};
    BookId book_id;
    std::string fingerprint;

    bool ok() const {
        return status == BookImportStatus::Ok;
    }
};

class BookImportService {
public:
    explicit BookImportService(LibraryService& library);

    PreparedBookImport prepare(
        const BookImportSource& source,
        std::uint64_t added_order
    );

    BookImportResult commit(
        const PreparedBookImport& prepared
    );

    BookImportResult import(
        const BookImportSource& source,
        std::uint64_t added_order
    );

private:
    LibraryService& library_;
    TxtParser txt_parser_;
    EpubParser epub_parser_;

    static std::string fingerprint(
        const std::string& bytes
    );

    static BookFormat detectFormat(
        const std::string& filename,
        bool& supported
    );
};

} // namespace enku
