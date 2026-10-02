#pragma once

#include "../services/services.hpp"
#include "book_file_store.hpp"

namespace enku {

class StoredBookSourceService final : public BookSourceService {
public:
    explicit StoredBookSourceService(BookFileStore& files);

    BookSourceStatus readSource(
        const BookRecord& record,
        std::string& bytes
    ) override;

    std::optional<std::uint64_t> sourceSize(
        const BookRecord& record
    ) override;

    BookSourceStatus readSourceRange(
        const BookRecord& record,
        std::uint64_t offset,
        std::size_t length,
        std::string& bytes
    ) override;

private:
    BookFileStore& files_;
};

} // namespace enku
