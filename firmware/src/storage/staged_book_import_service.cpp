#include "enku/storage/staged_book_import_service.hpp"

#include <algorithm>
#include <cctype>

namespace enku {

StagedBookImportService::StagedBookImportService(
    BookFileStore& files,
    BookImportService& importer,
    LibraryService& library
)
    : files_(files),
      importer_(importer),
      library_(library) {}

std::string StagedBookImportService::extension(
    const std::string& filename
) {
    const auto slash = filename.find_last_of("/\\");
    const auto base = slash == std::string::npos
        ? filename
        : filename.substr(slash + 1);

    const auto dot = base.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }

    std::string ext = base.substr(dot);
    std::transform(
        ext.begin(),
        ext.end(),
        ext.begin(),
        [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        }
    );
    return ext;
}

StagedImportStatus
StagedBookImportService::mapPreparedStatus(
    BookImportStatus status
) {
    switch (status) {
        case BookImportStatus::EmptySource:
            return StagedImportStatus::EmptySource;
        case BookImportStatus::UnsupportedFormat:
            return StagedImportStatus::UnsupportedFormat;
        case BookImportStatus::ParseFailed:
            return StagedImportStatus::ParseFailed;
        case BookImportStatus::Duplicate:
            return StagedImportStatus::Duplicate;
        case BookImportStatus::LibraryCommitFailed:
            return StagedImportStatus::LibraryCommitFailed;
        case BookImportStatus::Ok:
        default:
            return StagedImportStatus::Ok;
    }
}

StagedImportResult StagedBookImportService::import(
    const std::string& staged_path,
    const std::string& source_filename,
    std::uint64_t added_order
) {
    StagedImportResult result;

    std::string bytes;
    const auto read_status =
        files_.read(staged_path, bytes);

    if (read_status == BookFileStatus::NotFound) {
        result.status = StagedImportStatus::StageNotFound;
        return result;
    }

    if (read_status != BookFileStatus::Ok) {
        result.status = StagedImportStatus::StageReadFailed;
        return result;
    }

    BookImportSource source{
        staged_path,
        source_filename,
        bytes,
    };

    auto prepared =
        importer_.prepare(source, added_order);

    if (!prepared.ok()) {
        result.status =
            mapPreparedStatus(prepared.status);
        return result;
    }

    const auto ext = extension(source_filename);
    result.book_id = prepared.record.book_id;
    result.final_path =
        "/books/" + result.book_id + ext;

    prepared.record.source_path = result.final_path;

    const auto write_status =
        files_.write(result.final_path, bytes);

    if (write_status != BookFileStatus::Ok) {
        result.status =
            StagedImportStatus::FinalWriteFailed;
        return result;
    }

    const auto commit_result =
        importer_.commit(prepared);

    if (!commit_result.ok()) {
        const auto rollback_status =
            files_.remove(result.final_path);

        if (rollback_status != BookFileStatus::Ok) {
            result.status =
                StagedImportStatus::RollbackFailed;
            return result;
        }

        result.status =
            StagedImportStatus::LibraryCommitFailed;
        return result;
    }

    const auto cleanup_status =
        files_.remove(staged_path);

    if (cleanup_status != BookFileStatus::Ok) {
        result.status =
            StagedImportStatus::CleanupFailed;
        return result;
    }

    result.status = StagedImportStatus::Ok;
    return result;
}

} // namespace enku
