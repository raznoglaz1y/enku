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

    virtual BookFileStatus remove(
        const std::string& path
    ) = 0;

    virtual BookFileStatus list(
        const std::string& directory,
        std::vector<std::string>& paths
    ) = 0;
};

} // namespace enku
