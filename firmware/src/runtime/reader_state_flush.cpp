#include "enku/runtime/reader_state_flush.hpp"

namespace enku {

ReaderStateFlushCoordinator::ReaderStateFlushCoordinator(
    AppState& app_state,
    LibraryService& library,
    ReaderCheckpointService& checkpoint,
    AppContextService& context
)
    : app_state_(app_state),
      library_(library),
      checkpoint_(checkpoint),
      context_(context) {}

ReaderStateFlushStatus ReaderStateFlushCoordinator::flush(
    ReaderStateFlushTarget target
) {
    const bool has_active_book =
        app_state_.current_book.has_value();

    if (has_active_book) {
        const BookId book_id =
            *app_state_.current_book;

        const ReadingState reading_state =
            app_state_.current_book_finished
                ? ReadingState::Finished
                : ReadingState::Reading;

        if (app_state_.progress_dirty &&
            app_state_.reading_position.has_value()) {
            if (checkpoint_.checkpoint(
                    book_id,
                    *app_state_.reading_position,
                    app_state_.reading_progress,
                    reading_state
                ) != PersistStatus::Ok) {
                return ReaderStateFlushStatus::CheckpointFailed;
            }

            app_state_.progress_dirty = false;
        }

        const auto record =
            library_.get(book_id);

        if (!record.has_value() ||
            library_.updateSummary(
                book_id,
                reading_state,
                app_state_.reading_progress,
                record->last_opened_order
            ) != LibraryStatus::Ok) {
            return ReaderStateFlushStatus::LibraryUpdateFailed;
        }

        if (target ==
            ReaderStateFlushTarget::PreserveReading) {
            if (context_.save(
                    AppRestoreContext{
                        Screen::Reading,
                        book_id,
                        app_state_.library.persistedOffset(),
                        app_state_.library.persistedFocusedBook(),
                    }
                ) != PersistStatus::Ok) {
                return ReaderStateFlushStatus::ContextSaveFailed;
            }

            return ReaderStateFlushStatus::Applied;
        }
    }

    if (context_.save(
            AppRestoreContext{
                Screen::Library,
                std::nullopt,
                app_state_.library.persistedOffset(),
                app_state_.library.persistedFocusedBook(),
            }
        ) != PersistStatus::Ok) {
        return ReaderStateFlushStatus::ContextSaveFailed;
    }

    return ReaderStateFlushStatus::Applied;
}

} // namespace enku
