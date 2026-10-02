#include "enku/core/reader_overlay.hpp"

namespace enku {

ReadingPresetValues readingPresetValues(
    ReadingPreset preset
) {
    switch (preset) {
        case ReadingPreset::Spacious:
            return {20, 1.55F, 32};

        case ReadingPreset::Comfortable:
            return {19, 1.45F, 28};

        case ReadingPreset::Standard:
            return {18, 1.35F, 24};

        case ReadingPreset::Compact:
            return {17, 1.25F, 20};

        case ReadingPreset::Dense:
            return {16, 1.15F, 16};

        case ReadingPreset::Custom:
        default:
            return {18, 1.35F, 24};
    }
}

} // namespace enku
