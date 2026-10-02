#include "enku/storage/stored_book_source_service.hpp"

namespace enku {

StoredBookSourceService::StoredBookSourceService(
    BookFileStore& files
)
    : files_(files) {}

BookSourceStatus StoredBookSourceService::readSource(
    const BookRecord& record,
    std::string& bytes
) {
    const auto status =
        files_.read(record.source_path, bytes);

    switch (status) {
        case BookFileStatus::Ok:
            return BookSourceStatus::Ok;
        case BookFileStatus::NotFound:
            return BookSourceStatus::Unavailable;
        case BookFileStatus::IoError:
        case BookFileStatus::NoSpace:
        default:
            return BookSourceStatus::ReadFailed;
    }
}

BookSourceStatus
StoredBookSourceService::sourceSize(
    const BookRecord& record,
    std::uint64_t& size_bytes
) {
    const auto status =
        files_.size(
            record.source_path,
            size_bytes
        );

    switch (status) {
        case BookFileStatus::Ok:
            return BookSourceStatus::Ok;

        case BookFileStatus::NotFound:
            size_bytes = 0;
            return BookSourceStatus::Unavailable;

        case BookFileStatus::IoError:
        case BookFileStatus::NoSpace:
        default:
            size_bytes = 0;
            return BookSourceStatus::ReadFailed;
    }
}

BookSourceStatus
StoredBookSourceService::readSourceRange(
    const BookRecord& record,
    std::uint64_t offset,
    std::size_t length,
    std::string& bytes
) {
    const auto status =
        files_.readRange(
            record.source_path,
            offset,
            length,
            bytes
        );

    switch (status) {
        case BookFileStatus::Ok:
            return BookSourceStatus::Ok;

        case BookFileStatus::NotFound:
            return BookSourceStatus::Unavailable;

        case BookFileStatus::IoError:
        case BookFileStatus::NoSpace:
        default:
            return BookSourceStatus::ReadFailed;
    }
}


} // namespace enku
