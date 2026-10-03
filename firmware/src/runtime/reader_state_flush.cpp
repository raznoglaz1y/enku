#include "enku/runtime/reader_state_flush.hpp"

namespace enku {
namespace {

bool isReadingContext(Screen screen) {
    switch (screen) {
        case Screen::Reading:
        case Screen::ReaderOverlay:
        case Screen::Search:
        case Screen::BookFinished:
        case Screen::ContentsBookmarks:
        case Screen::AboutBook:
        case Screen::ReadingSettings:
            return true;
        default:
            return false;
    }
}

} // namespace

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

ReaderStateFlushStatus
ReaderStateFlushCoordinator::checkpointProgress() {
    if (!app_state_.current_book.has_value() ||
        !app_state_.reading_position.has_value() ||
        !app_state_.progress_dirty) {
        return ReaderStateFlushStatus::Applied;
    }

    const ReadingState reading_state =
        app_state_.current_book_finished
            ? ReadingState::Finished
            : ReadingState::Reading;

    if (checkpoint_.checkpoint(
            *app_state_.current_book,
            *app_state_.reading_position,
            app_state_.reading_progress,
            reading_state
        ) != PersistStatus::Ok) {
        return ReaderStateFlushStatus::CheckpointFailed;
    }

    app_state_.progress_dirty = false;
    return ReaderStateFlushStatus::Applied;
}

ReaderStateFlushStatus ReaderStateFlushCoordinator::flush(
    ReaderStateFlushTarget target
) {
    const bool has_active_book =
        app_state_.current_book.has_value() &&
        isReadingContext(app_state_.screen);

    if (has_active_book) {
        const BookId book_id =
            *app_state_.current_book;

        const ReadingState reading_state =
            app_state_.current_book_finished
                ? ReadingState::Finished
                : ReadingState::Reading;

        if (checkpointProgress() !=
            ReaderStateFlushStatus::Applied) {
            return ReaderStateFlushStatus::CheckpointFailed;
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
