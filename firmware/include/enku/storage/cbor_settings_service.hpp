#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../services/services.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborSettingsService final : public SettingsService {
public:
    explicit CborSettingsService(StateFileStore& files);

    PersistStatus load(
        GlobalSettings& settings
    ) override;

    PersistStatus save(
        const GlobalSettings& settings
    ) override;

private:
    struct DecodedSettings {
        std::uint32_t generation{0};
        GlobalSettings settings;
    };

    StateFileStore& files_;

    static constexpr const char* kSlotA =
        "/system/settings.a.cbor";
    static constexpr const char* kSlotB =
        "/system/settings.b.cbor";

    PersistStatus readSlot(
        const std::string& path,
        DecodedSettings& decoded
    );

    static bool valid(
        const GlobalSettings& settings
    );

    static std::vector<std::uint8_t> encode(
        std::uint32_t generation,
        const GlobalSettings& settings
    );

    static bool decode(
        const std::vector<std::uint8_t>& bytes,
        DecodedSettings& decoded
    );
};

} // namespace enku
