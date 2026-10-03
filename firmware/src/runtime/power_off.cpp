#include "enku/runtime/power_off.hpp"
#include "enku/runtime/reader_state_flush.hpp"

namespace enku {

PowerOffCoordinator::PowerOffCoordinator(
    AppState& app_state,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    AppContextService& context,
    NetworkService& network,
    PowerService& power
)
    : app_state_(app_state),
      library_(library),
      checkpoint_(checkpoint),
      context_(context),
      network_(network),
      power_(power) {}

PowerOffStatus PowerOffCoordinator::powerOff() {
    if (app_state_.import_active) {
        return PowerOffStatus::BusyImport;
    }

    ReaderStateFlushCoordinator flush(
        app_state_,
        library_,
        checkpoint_,
        context_
    );

    switch (flush.flush(
        ReaderStateFlushTarget::PreserveReading
    )) {
        case ReaderStateFlushStatus::Applied:
            break;
        case ReaderStateFlushStatus::CheckpointFailed:
            return PowerOffStatus::CheckpointFailed;
        case ReaderStateFlushStatus::LibraryUpdateFailed:
            return PowerOffStatus::LibraryUpdateFailed;
        case ReaderStateFlushStatus::ContextSaveFailed:
            return PowerOffStatus::ContextSaveFailed;
    }

    if (network_.connected()) {
        network_.disconnect();
    }

    power_.requestPowerOff();
    return PowerOffStatus::Applied;
}

} // namespace enku
