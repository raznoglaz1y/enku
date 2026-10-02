#pragma once

#include <cstdint>
#include <string>

#include "book_file_store.hpp"
#include "book_import_service.hpp"

namespace enku {

enum class StagedImportStatus : std::uint8_t {
    Ok,
    StageNotFound,
    StageReadFailed,
    EmptySource,
    UnsupportedFormat,
    ParseFailed,
    Duplicate,
    FinalWriteFailed,
    LibraryCommitFailed,
    RollbackFailed,
    CleanupFailed,
};

struct StagedImportResult {
    StagedImportStatus status{StagedImportStatus::StageReadFailed};
    BookId book_id;
    std::string final_path;

    bool ok() const {
        return status == StagedImportStatus::Ok;
    }
};

class StagedBookImportService {
public:
    StagedBookImportService(
        BookFileStore& files,
        BookImportService& importer,
        LibraryService& library
    );

    StagedImportResult import(
        const std::string& staged_path,
        const std::string& source_filename,
        std::uint64_t added_order
    );

private:
    BookFileStore& files_;
    BookImportService& importer_;
    LibraryService& library_;

    static std::string extension(
        const std::string& filename
    );

    static StagedImportStatus mapPreparedStatus(
        BookImportStatus status
    );
};

} // namespace enku
