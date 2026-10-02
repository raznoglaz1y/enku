#pragma once

#include <filesystem>
#include <string>

#include "state_file_store.hpp"

namespace enku {

class PosixStateFileStore final : public StateFileStore {
public:
    explicit PosixStateFileStore(std::filesystem::path root);

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

    const std::filesystem::path& root() const;

private:
    std::filesystem::path root_;

    std::filesystem::path resolve(
        const std::string& path
    ) const;
};

} // namespace enku
