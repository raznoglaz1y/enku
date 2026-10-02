#pragma once

#include <filesystem>
#include <string>

#include "book_file_store.hpp"

namespace enku {

class PosixBookFileStore final : public BookFileStore {
public:
    explicit PosixBookFileStore(std::filesystem::path root);

    BookFileStatus read(
        const std::string& path,
        std::string& bytes
    ) override;

    BookFileStatus write(
        const std::string& path,
        const std::string& bytes
    ) override;

    BookFileStatus size(
        const std::string& path,
        std::uint64_t& bytes
    ) override;

    BookFileStatus readRange(
        const std::string& path,
        std::uint64_t offset,
        std::size_t length,
        std::string& bytes
    ) override;

    BookFileStatus append(
        const std::string& path,
        const std::string& bytes
    ) override;

    BookFileStatus move(
        const std::string& from,
        const std::string& to
    ) override;

    BookFileStatus remove(
        const std::string& path
    ) override;

    BookFileStatus list(
        const std::string& directory,
        std::vector<std::string>& paths
    ) override;

private:
    std::filesystem::path root_;

    std::filesystem::path resolve(
        const std::string& path
    ) const;
};

} // namespace enku
