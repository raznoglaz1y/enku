#pragma once

#include <optional>
#include <vector>

#include "reader_engine.hpp"

namespace enku {

enum class ReaderSessionStatus : std::uint8_t {
    Closed,
    Ready,
    EndOfBook,
    BeginningOfBook,
    LayoutFailed,
};

class ReaderSession {
public:
    explicit ReaderSession(ReaderEngine& engine);

    ReaderSessionStatus open(const LayoutRequest& request);
    ReaderSessionStatus next();
    ReaderSessionStatus previous();

    void invalidateLayout(
        const TypographySettings& typography,
        const Viewport& viewport
    );

    bool isOpen() const;
    const std::optional<PageResult>& currentPage() const;
    ReaderSessionStatus status() const;

private:
    ReaderEngine& engine_;

    std::optional<PageResult> previous_page_;
    std::optional<PageResult> current_page_;
    std::optional<PageResult> next_page_;

    std::vector<SemanticPosition> history_;
    LayoutRequest request_;
    ReaderSessionStatus status_{ReaderSessionStatus::Closed};

    std::optional<PageResult> layoutAt(const SemanticPosition& anchor);
    void prefetchNext();
    void prefetchPrevious();
    void applyHistoryAnchors(PageResult& page) const;

    static bool samePosition(
        const SemanticPosition& a,
        const SemanticPosition& b
    );
};

} // namespace enku
