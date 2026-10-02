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
