#pragma once

#include <cstdint>

#include "../core/app_state.hpp"
#include "../core/events.hpp"
#include "../core/refresh.hpp"
#include "../reader/book_loader.hpp"
#include "../render/reader_page_renderer.hpp"
#include "../services/services.hpp"

namespace enku {

enum class ReaderRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    BookOpening,
    BookOpenFailed,
    BeginningOfBook,
    EndOfBook,
    LayoutFailed,
    CheckpointFailed,
    CheckpointLoadFailed,
    ContextSaveFailed,
    LibraryUpdateFailed,
    RefreshRejected,
    RenderFailed,
};

class ReaderRuntimeController {
public:
    ReaderRuntimeController(
        AppState& app_state,
        ReaderBookLoader& loader,
        RefreshService& refresh,
        LibraryService& library,
        ReaderCheckpointService& checkpoint,
        AppContextService& context,
        TypographySettings typography,
        Viewport viewport,
        ReaderPageRenderer* page_renderer = nullptr
    );

    ReaderRuntimeResult handle(const OpenBookRequested&);
    ReaderRuntimeResult handle(const BookOpened&);
    ReaderRuntimeResult handle(const BookOpenFailed&);
    ReaderRuntimeResult handle(const PageNextRequested&);
    ReaderRuntimeResult handle(const PagePreviousRequested&);
    ReaderRuntimeResult handle(const BackRequested&);

    std::uint32_t refreshGeneration() const;

private:
    AppState& app_state_;
    ReaderBookLoader& loader_;
    RefreshService& refresh_;
    LibraryService& library_;
    ReaderCheckpointService& checkpoint_;
    AppContextService& context_;
    TypographySettings typography_;
    Viewport native_viewport_;
    Viewport viewport_;
    ReaderPageRenderer* page_renderer_{nullptr};
    std::uint32_t refresh_generation_{0};

    ReaderSession* session();
    const ReaderSession* session() const;

    ReaderRuntimeResult applySessionResult(
        ReaderSessionStatus status
    );

    ReaderRuntimeResult commitVisiblePage(
        bool mark_progress_dirty = true
    );

    ReaderRuntimeResult submitRefresh(
        RefreshReason reason,
        bool may_coalesce = false,
        bool may_defer = false
    );

    Viewport orientedViewport() const;

    LibraryStatus updateLibrarySummary(
        ReadingState reading_state,
        float progress
    );
};

} // namespace enku
