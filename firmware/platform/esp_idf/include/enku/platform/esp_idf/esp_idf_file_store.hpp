#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "enku/storage/book_file_store.hpp"
#include "enku/storage/state_file_store.hpp"

namespace enku::platform::esp_idf {

enum class EspIdfFsStatus : std::uint8_t {
    Ok,
    NotFound,
    IoError,
    NoSpace,
};

class EspIdfFilesystem {
public:
    explicit EspIdfFilesystem(
        std::string mount_point = "/sdcard"
    );

    EspIdfFsStatus readBytes(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    );

    EspIdfFsStatus writeBytes(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    );

    EspIdfFsStatus readText(
        const std::string& path,
        std::string& bytes
    );

    EspIdfFsStatus writeText(
        const std::string& path,
        const std::string& bytes
    );

    EspIdfFsStatus appendText(
        const std::string& path,
        const std::string& bytes
    );

    EspIdfFsStatus remove(
        const std::string& path
    );

    EspIdfFsStatus list(
        const std::string& directory,
        std::vector<std::string>& paths
    );

private:
    std::string mount_point_;

    bool resolve(
        const std::string& logical_path,
        std::string& physical_path
    ) const;

    bool ensureParentDirectories(
        const std::string& physical_path
    ) const;
};

class EspIdfStateFileStore final : public StateFileStore {
public:
    explicit EspIdfStateFileStore(
        EspIdfFilesystem& filesystem
    );

    StateFileStatus read(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    ) override;

    StateFileStatus write(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    ) override;

    StateFileStatus remove(
        const std::string& path
    ) override;

private:
    EspIdfFilesystem& filesystem_;
};

class EspIdfBookFileStore final : public BookFileStore {
public:
    explicit EspIdfBookFileStore(
        EspIdfFilesystem& filesystem
    );

    BookFileStatus read(
        const std::string& path,
        std::string& bytes
    ) override;

    BookFileStatus write(
        const std::string& path,
        const std::string& bytes
    ) override;

    BookFileStatus append(
        const std::string& path,
        const std::string& bytes
    ) override;

    BookFileStatus remove(
        const std::string& path
    ) override;

    BookFileStatus list(
        const std::string& directory,
        std::vector<std::string>& paths
    ) override;

private:
    EspIdfFilesystem& filesystem_;
};

} // namespace enku::platform::esp_idf
