#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../services/services.hpp"

namespace enku {

enum class ReaderStateFlushTarget : std::uint8_t {
    PreserveReading,
    ReturnToLibrary,
};

enum class ReaderStateFlushStatus : std::uint8_t {
    Applied,
    CheckpointFailed,
    LibraryUpdateFailed,
    ContextSaveFailed,
};

class ReaderStateFlushCoordinator {
public:
    ReaderStateFlushCoordinator(
        AppState& app_state,
        LibraryService& library,
        ReaderCheckpointService& checkpoint,
        AppContextService& context
    );

    ReaderStateFlushStatus flush(
        ReaderStateFlushTarget target
    );

private:
    AppState& app_state_;
    LibraryService& library_;
    ReaderCheckpointService& checkpoint_;
    AppContextService& context_;
};

} // namespace enku
