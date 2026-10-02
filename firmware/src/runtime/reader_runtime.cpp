#include "enku/runtime/reader_runtime.hpp"

namespace enku {

ReaderRuntimeController::ReaderRuntimeController(
    AppState& app_state,
    ReaderSession& session,
    RefreshService& refresh
)
    : app_state_(app_state),
      session_(session),
      refresh_(refresh) {}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const PageNextRequested&
) {
    if (app_state_.screen != Screen::Reading ||
        !session_.isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    return applySessionResult(session_.next());
}

ReaderRuntimeResult ReaderRuntimeController::handle(
    const PagePreviousRequested&
) {
    if (app_state_.screen != Screen::Reading ||
        !session_.isOpen()) {
        return ReaderRuntimeResult::Ignored;
    }

    return applySessionResult(session_.previous());
}

ReaderRuntimeResult ReaderRuntimeController::applySessionResult(
    ReaderSessionStatus status
) {
    switch (status) {
        case ReaderSessionStatus::Ready:
            return commitVisiblePage();

        case ReaderSessionStatus::BeginningOfBook:
            return ReaderRuntimeResult::BeginningOfBook;

        case ReaderSessionStatus::EndOfBook:
            return ReaderRuntimeResult::EndOfBook;

        case ReaderSessionStatus::LayoutFailed:
            return ReaderRuntimeResult::LayoutFailed;

        case ReaderSessionStatus::Closed:
        default:
            return ReaderRuntimeResult::Ignored;
    }
}

ReaderRuntimeResult ReaderRuntimeController::commitVisiblePage() {
    const auto& current = session_.currentPage();
    if (!current.has_value()) {
        return ReaderRuntimeResult::LayoutFailed;
    }

    app_state_.current_book = current->first_position.book_id;
    app_state_.reading_position = current->first_position;
    app_state_.reading_progress = current->progress;
    app_state_.progress_dirty = true;

    RefreshRequest refresh_request;
    refresh_request.refresh_class = RefreshClass::Full;
    refresh_request.reason = RefreshReason::PageTurn;
    refresh_request.generation = ++refresh_generation_;
    refresh_request.may_coalesce = false;
    refresh_request.may_defer = false;

    if (!refresh_.submit(refresh_request)) {
        // Logical AppState remains authoritative even if the display update
        // cannot be queued. Display recovery is handled separately.
        return ReaderRuntimeResult::RefreshRejected;
    }

    return ReaderRuntimeResult::Applied;
}

std::uint32_t ReaderRuntimeController::refreshGeneration() const {
    return refresh_generation_;
}

} // namespace enku
