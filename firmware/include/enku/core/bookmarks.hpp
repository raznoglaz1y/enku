#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "types.hpp"

namespace enku {

struct BookmarkRecord {
    SemanticPosition position;
    std::string label;
};

enum class BookmarkStatus : std::uint8_t {
    Ok,
    NotFound,
    AlreadyExists,
    Invalid,
    IoError,
    Corrupt,
};

} // namespace enku
