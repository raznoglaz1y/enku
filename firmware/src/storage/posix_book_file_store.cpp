#include "enku/storage/posix_book_file_store.hpp"

#include <fstream>
#include <system_error>
#include <utility>

namespace enku {

PosixBookFileStore::PosixBookFileStore(
    std::filesystem::path root
)
    : root_(std::move(root)) {}

std::filesystem::path
PosixBookFileStore::resolve(
    const std::string& path
) const {
    std::filesystem::path relative(path);

    if (relative.is_absolute()) {
        relative = relative.relative_path();
    }

    return root_ / relative;
}

BookFileStatus PosixBookFileStore::read(
    const std::string& path,
    std::string& bytes
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    if (!std::filesystem::exists(full_path, ec)) {
        return ec
            ? BookFileStatus::IoError
            : BookFileStatus::NotFound;
    }

    std::ifstream input(
        full_path,
        std::ios::binary | std::ios::ate
    );
    if (!input) {
        return BookFileStatus::IoError;
    }

    const auto size = input.tellg();
    if (size < 0) {
        return BookFileStatus::IoError;
    }

    bytes.resize(static_cast<std::size_t>(size));
    input.seekg(0, std::ios::beg);

    if (!bytes.empty()) {
        input.read(
            bytes.data(),
            static_cast<std::streamsize>(bytes.size())
        );
        if (!input) {
            return BookFileStatus::IoError;
        }
    }

    return BookFileStatus::Ok;
}

BookFileStatus PosixBookFileStore::write(
    const std::string& path,
    const std::string& bytes
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    std::filesystem::create_directories(
        full_path.parent_path(),
        ec
    );
    if (ec) {
        return BookFileStatus::IoError;
    }

    std::ofstream output(
        full_path,
        std::ios::binary | std::ios::trunc
    );
    if (!output) {
        return BookFileStatus::IoError;
    }

    if (!bytes.empty()) {
        output.write(
            bytes.data(),
            static_cast<std::streamsize>(bytes.size())
        );
    }
    output.flush();

    return output
        ? BookFileStatus::Ok
        : BookFileStatus::IoError;
}

BookFileStatus PosixBookFileStore::remove(
    const std::string& path
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    const bool removed =
        std::filesystem::remove(full_path, ec);

    if (ec) {
        return BookFileStatus::IoError;
    }

    return removed
        ? BookFileStatus::Ok
        : BookFileStatus::NotFound;
}

} // namespace enku
