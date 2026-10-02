#include "enku/runtime/power_off.hpp"

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

    if (app_state_.screen == Screen::Reading &&
        app_state_.current_book.has_value()) {
        const BookId book_id = *app_state_.current_book;
        const ReadingState reading_state =
            app_state_.current_book_finished
                ? ReadingState::Finished
                : ReadingState::Reading;

        if (app_state_.reading_position.has_value()) {
            if (checkpoint_.checkpoint(
                    book_id,
                    *app_state_.reading_position,
                    app_state_.reading_progress,
                    reading_state
                ) != PersistStatus::Ok) {
                return PowerOffStatus::CheckpointFailed;
            }

            app_state_.progress_dirty = false;
        }

        const auto record = library_.get(book_id);
        if (!record.has_value()) {
            return PowerOffStatus::LibraryUpdateFailed;
        }

        if (library_.updateSummary(
                book_id,
                reading_state,
                app_state_.reading_progress,
                record->last_opened_order
            ) != LibraryStatus::Ok) {
            return PowerOffStatus::LibraryUpdateFailed;
        }

        if (context_.save(
                AppRestoreContext{
                    Screen::Reading,
                    book_id,
                    app_state_.library.offset,
                    app_state_.library.focused_book,
                }
            ) != PersistStatus::Ok) {
            return PowerOffStatus::ContextSaveFailed;
        }
    } else {
        if (context_.save(
                AppRestoreContext{
                    Screen::Library,
                    std::nullopt,
                    app_state_.library.offset,
                    app_state_.library.focused_book,
                }
            ) != PersistStatus::Ok) {
            return PowerOffStatus::ContextSaveFailed;
        }
    }

    if (network_.connected()) {
        network_.disconnect();
    }

    power_.requestPowerOff();
    return PowerOffStatus::Applied;
}

} // namespace enku
