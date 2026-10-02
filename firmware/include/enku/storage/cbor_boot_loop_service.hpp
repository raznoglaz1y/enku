#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../services/services.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborBootLoopService final : public BootLoopService {
public:
    explicit CborBootLoopService(StateFileStore& files);

    PersistStatus beginBoot(
        BootLoopMarker& marker
    ) override;

    PersistStatus markStable() override;

private:
    struct DecodedMarker {
        std::uint32_t generation{0};
        BootLoopMarker marker;
    };

    StateFileStore& files_;

    static constexpr const char* kSlotA =
        "/system/boot-marker.a.cbor";
    static constexpr const char* kSlotB =
        "/system/boot-marker.b.cbor";

    PersistStatus load(
        BootLoopMarker& marker
    );

    PersistStatus save(
        const BootLoopMarker& marker
    );

    PersistStatus readSlot(
        const std::string& path,
        DecodedMarker& decoded
    );

    static std::vector<std::uint8_t> encode(
        std::uint32_t generation,
        const BootLoopMarker& marker
    );

    static bool decode(
        const std::vector<std::uint8_t>& bytes,
        DecodedMarker& decoded
    );
};

} // namespace enku
