#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace enku {

enum class StateFileStatus : std::uint8_t {
    Ok,
    NotFound,
    IoError,
    NoSpace,
};

class StateFileStore {
public:
    virtual ~StateFileStore() = default;

    virtual StateFileStatus read(
        const std::string& path,
        std::vector<std::uint8_t>& bytes
    ) = 0;

    virtual StateFileStatus write(
        const std::string& path,
        const std::vector<std::uint8_t>& bytes
    ) = 0;
};

} // namespace enku
