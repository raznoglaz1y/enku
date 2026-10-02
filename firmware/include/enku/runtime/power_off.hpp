#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../services/services.hpp"

namespace enku {

enum class PowerOffStatus : std::uint8_t {
    Applied,
    BusyImport,
    CheckpointFailed,
    LibraryUpdateFailed,
    ContextSaveFailed,
};

class PowerOffCoordinator {
public:
    PowerOffCoordinator(
        AppState& app_state,
        LibraryService& library,
        ReaderCheckpointService& checkpoint,
        AppContextService& context,
        NetworkService& network,
        PowerService& power
    );

    PowerOffStatus powerOff();

private:
    AppState& app_state_;
    LibraryService& library_;
    ReaderCheckpointService& checkpoint_;
    AppContextService& context_;
    NetworkService& network_;
    PowerService& power_;
};

} // namespace enku
