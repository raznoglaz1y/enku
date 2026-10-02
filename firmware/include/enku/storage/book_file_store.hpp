#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace enku {

enum class BookFileStatus : std::uint8_t {
    Ok,
    NotFound,
    IoError,
    NoSpace,
};

class BookFileStore {
public:
    virtual ~BookFileStore() = default;

    virtual BookFileStatus read(
        const std::string& path,
        std::string& bytes
    ) = 0;

    virtual BookFileStatus write(
        const std::string& path,
        const std::string& bytes
    ) = 0;

    virtual BookFileStatus size(
        const std::string& path,
        std::uint64_t& bytes
    ) {
        std::string content;
        const auto status =
            read(path, content);

        if (status != BookFileStatus::Ok) {
            bytes = 0;
            return status;
        }

        bytes =
            static_cast<std::uint64_t>(
                content.size()
            );
        return BookFileStatus::Ok;
    }

    virtual BookFileStatus readRange(
        const std::string& path,
        std::uint64_t offset,
        std::size_t length,
        std::string& bytes
    ) {
        std::string content;
        const auto status =
            read(path, content);

        if (status != BookFileStatus::Ok) {
            bytes.clear();
            return status;
        }

        if (offset >
                static_cast<std::uint64_t>(
                    content.size()
                ) ||
            static_cast<std::uint64_t>(
                length
            ) >
                static_cast<std::uint64_t>(
                    content.size()
                ) -
                    offset) {
            bytes.clear();
            return BookFileStatus::IoError;
        }

        bytes.assign(
            content,
            static_cast<std::size_t>(
                offset
            ),
            length
        );

        return BookFileStatus::Ok;
    }

    virtual BookFileStatus append(
        const std::string& path,
        const std::string& bytes
    ) {
        std::string existing;
        const auto status = read(path, existing);

        if (status != BookFileStatus::Ok &&
            status != BookFileStatus::NotFound) {
            return status;
        }

        existing.append(bytes);
        return write(path, existing);
    }

    virtual BookFileStatus move(
        const std::string& from,
        const std::string& to
    ) {
        std::string bytes;
        const auto read_status =
            read(from, bytes);

        if (read_status != BookFileStatus::Ok) {
            return read_status;
        }

        const auto write_status =
            write(to, bytes);

        if (write_status != BookFileStatus::Ok) {
            return write_status;
        }

        return remove(from);
    }

    virtual BookFileStatus remove(
        const std::string& path
    ) = 0;

    virtual BookFileStatus list(
        const std::string& directory,
        std::vector<std::string>& paths
    ) = 0;
};

} // namespace enku
