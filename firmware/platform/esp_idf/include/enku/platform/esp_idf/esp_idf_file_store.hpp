#pragma once

#include <string>
#include <vector>

#include "enku/storage/book_file_store.hpp"
#include "enku/storage/state_file_store.hpp"

namespace enku::platform::esp_idf {

class EspIdfFileStore final
    : public StateFileStore,
      public BookFileStore {
public:
    explicit EspIdfFileStore(
        std::string mount_point = "/sdcard"
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

    BookFileStatus read(
        const std::string& path,
        std::string& bytes
    ) override;

    BookFileStatus write(
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
    std::string mount_point_;

    bool resolve(
        const std::string& logical_path,
        std::string& physical_path
    ) const;

    bool ensureParentDirectories(
        const std::string& physical_path
    ) const;
};

} // namespace enku::platform::esp_idf
