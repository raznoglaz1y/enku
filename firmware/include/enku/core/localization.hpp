#pragma once

#include <cstdint>
#include <string_view>

namespace enku {

enum class LocaleId : std::uint8_t {
    En,
    Pl,
    De,
    Fr,
    Es,
    It,
    Ru,
};

enum class PluralCategory : std::uint8_t {
    One,
    Few,
    Many,
    Other,
};

using StringKey = std::uint32_t;

struct LocaleInfo {
    LocaleId id{LocaleId::En};
    std::string_view tag{"en"};
    std::string_view native_name{"English"};
};

} // namespace enku
