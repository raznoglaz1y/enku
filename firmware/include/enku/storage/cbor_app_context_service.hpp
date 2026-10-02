#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../services/services.hpp"
#include "state_file_store.hpp"

namespace enku {

class CborAppContextService final : public AppContextService {
public:
    explicit CborAppContextService(StateFileStore& files);

    PersistStatus load(
        AppRestoreContext& context
    ) override;

    PersistStatus save(
        const AppRestoreContext& context
    ) override;

private:
    struct DecodedContext {
        std::uint32_t generation{0};
        AppRestoreContext context;
    };

    StateFileStore& files_;

    static constexpr const char* kSlotA =
        "/system/context.a.cbor";
    static constexpr const char* kSlotB =
        "/system/context.b.cbor";

    PersistStatus readSlot(
        const std::string& path,
        DecodedContext& decoded
    );

    static std::vector<std::uint8_t> encode(
        std::uint32_t generation,
        const AppRestoreContext& context
    );

    static bool decode(
        const std::vector<std::uint8_t>& bytes,
        DecodedContext& decoded
    );
};

} // namespace enku
