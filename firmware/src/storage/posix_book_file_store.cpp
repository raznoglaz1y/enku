#include "enku/storage/posix_book_file_store.hpp"

#include <algorithm>
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

BookFileStatus PosixBookFileStore::size(
    const std::string& path,
    std::uint64_t& bytes
) {
    const auto full_path = resolve(path);

    std::error_code ec;
    const auto value =
        std::filesystem::file_size(
            full_path,
            ec
        );

    if (ec) {
        bytes = 0;
        return ec ==
                std::errc::no_such_file_or_directory
            ? BookFileStatus::NotFound
            : BookFileStatus::IoError;
    }

    bytes =
        static_cast<std::uint64_t>(
            value
        );
    return BookFileStatus::Ok;
}

BookFileStatus PosixBookFileStore::readRange(
    const std::string& path,
    std::uint64_t offset,
    std::size_t length,
    std::string& bytes
) {
    bytes.clear();

    const auto full_path = resolve(path);

    std::ifstream input(
        full_path,
        std::ios::binary
    );

    if (!input) {
        std::error_code ec;
        return std::filesystem::exists(
                   full_path,
                   ec
               )
            ? BookFileStatus::IoError
            : BookFileStatus::NotFound;
    }

    input.seekg(
        static_cast<std::streamoff>(
            offset
        ),
        std::ios::beg
    );

    if (!input) {
        return BookFileStatus::IoError;
    }

    bytes.resize(length);

    if (length != 0U) {
        input.read(
            bytes.data(),
            static_cast<std::streamsize>(
                length
            )
        );

        if (input.gcount() !=
            static_cast<std::streamsize>(
                length
            )) {
            bytes.clear();
            return BookFileStatus::IoError;
        }
    }

    return BookFileStatus::Ok;
}

BookFileStatus PosixBookFileStore::append(
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
        std::ios::binary | std::ios::app
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

BookFileStatus PosixBookFileStore::move(
    const std::string& from,
    const std::string& to
) {
    const auto source = resolve(from);
    const auto target = resolve(to);

    std::error_code ec;
    if (!std::filesystem::exists(source, ec)) {
        return ec
            ? BookFileStatus::IoError
            : BookFileStatus::NotFound;
    }

    std::filesystem::create_directories(
        target.parent_path(),
        ec
    );
    if (ec) {
        return BookFileStatus::IoError;
    }

    std::filesystem::rename(
        source,
        target,
        ec
    );

    return ec
        ? BookFileStatus::IoError
        : BookFileStatus::Ok;
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

BookFileStatus PosixBookFileStore::list(
    const std::string& directory,
    std::vector<std::string>& paths
) {
    paths.clear();
    const auto full_dir = resolve(directory);

    std::error_code ec;
    if (!std::filesystem::exists(full_dir, ec)) {
        return ec
            ? BookFileStatus::IoError
            : BookFileStatus::NotFound;
    }

    if (!std::filesystem::is_directory(full_dir, ec) || ec) {
        return BookFileStatus::IoError;
    }

    for (const auto& entry :
         std::filesystem::directory_iterator(full_dir, ec)) {
        if (ec) {
            return BookFileStatus::IoError;
        }

        if (!entry.is_regular_file()) {
            continue;
        }

        const auto relative =
            std::filesystem::relative(entry.path(), root_, ec);

        if (ec) {
            return BookFileStatus::IoError;
        }

        paths.push_back("/" + relative.generic_string());
    }

    std::sort(paths.begin(), paths.end());
    return BookFileStatus::Ok;
}

} // namespace enku
