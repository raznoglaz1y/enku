#include "enku/runtime/about_book_runtime.hpp"

namespace enku {

AboutBookRuntime::AboutBookRuntime(
    AppState& app_state,
    LibraryService& library,
    ApplicationReaderRuntime& reader,
    AboutBookRenderer* renderer,
    RefreshService* refresh
)
    : app_state_(app_state),
      library_(library),
      reader_(reader),
      renderer_(renderer),
      refresh_(refresh) {}

AboutBookRuntimeResult
AboutBookRuntime::openFromReader() {
    if (app_state_.screen != Screen::Reading ||
        !app_state_.current_book.has_value()) {
        return AboutBookRuntimeResult::Ignored;
    }

    if (!library_.get(
            *app_state_.current_book
        ).has_value()) {
        return AboutBookRuntimeResult::Failed;
    }

    app_state_.screen = Screen::AboutBook;
    return render();
}

AboutBookRuntimeResult
AboutBookRuntime::render() {
    if (!app_state_.current_book.has_value()) {
        return AboutBookRuntimeResult::Failed;
    }

    const auto book =
        library_.get(
            *app_state_.current_book
        );

    if (!book.has_value()) {
        return AboutBookRuntimeResult::Failed;
    }

    if (renderer_ != nullptr &&
        !renderer_->renderAboutBook(
            app_state_,
            *book
        )) {
        return AboutBookRuntimeResult::Failed;
    }

    if (refresh_ == nullptr) {
        return AboutBookRuntimeResult::Applied;
    }

    RefreshRequest request;
    request.refresh_class = RefreshClass::Full;
    request.reason = RefreshReason::OverlayChanged;
    request.generation = ++refresh_generation_;
    request.may_coalesce = false;
    request.may_defer = false;

    return refresh_->submit(request)
        ? AboutBookRuntimeResult::Applied
        : AboutBookRuntimeResult::Failed;
}

AboutBookRuntimeResult
AboutBookRuntime::close() {
    app_state_.screen = Screen::Reading;

    const auto redraw =
        reader_.reader().redrawCurrentPage(
            RefreshReason::OverlayChanged
        );

    if (redraw != ReaderRuntimeResult::Applied) {
        app_state_.screen = Screen::AboutBook;
        return AboutBookRuntimeResult::Failed;
    }

    return AboutBookRuntimeResult::Applied;
}

AboutBookRuntimeResult
AboutBookRuntime::handle(
    LogicalAction action
) {
    if (app_state_.screen != Screen::AboutBook) {
        return AboutBookRuntimeResult::Ignored;
    }

    if (action == LogicalAction::Back ||
        action == LogicalAction::Confirm) {
        return close();
    }

    return AboutBookRuntimeResult::Ignored;
}

} // namespace enku
