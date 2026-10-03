#include "enku/core/board_profile.hpp"

#include <cassert>

using namespace enku;

int main() {
    BoardCapabilities capabilities;
    capabilities.has_psram = true;
    capabilities.has_removable_storage = true;
    capabilities.has_native_usb = true;
    capabilities.display_width = 800;
    capabilities.display_height = 480;
    capabilities.portrait = true;
    capabilities.landscape = true;
    capabilities.inverted_portrait = true;
    capabilities.inverted_landscape = false;

    assert(capabilities.has_psram);
    assert(capabilities.has_removable_storage);
    assert(capabilities.display_width == 800U);
    assert(capabilities.display_height == 480U);

    assert(
        supportsOrientation(
            capabilities,
            ReaderOrientation::Portrait
        )
    );
    assert(
        supportsOrientation(
            capabilities,
            ReaderOrientation::Landscape
        )
    );
    assert(
        supportsOrientation(
            capabilities,
            ReaderOrientation::PortraitInverted
        )
    );
    assert(
        !supportsOrientation(
            capabilities,
            ReaderOrientation::LandscapeInverted
        )
    );

    const BoardProfile profile{
        BoardProfileId::EnkuR01Base,
        BoardProfileMaturity::DesignTarget,
        "ENKU R0.1 Base",
        capabilities,
    };

    assert(profile.id == BoardProfileId::EnkuR01Base);
    assert(
        profile.maturity ==
        BoardProfileMaturity::DesignTarget
    );
    assert(profile.name == "ENKU R0.1 Base");

    return 0;
}
