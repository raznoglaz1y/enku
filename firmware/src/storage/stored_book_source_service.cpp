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

} // namespace enku


std::optional<std::uint64_t>
StoredBookSourceService::sourceSize(
    const BookRecord& record
) {
    std::uint64_t bytes = 0;

    const auto status =
        files_.size(
            record.source_path,
            bytes
        );

    return status == BookFileStatus::Ok
        ? std::optional<std::uint64_t>{
              bytes
          }
        : std::nullopt;
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
