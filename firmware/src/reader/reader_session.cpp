#include "enku/reader/reader_session.hpp"

namespace enku {

ReaderSession::ReaderSession(ReaderEngine& engine)
    : engine_(engine) {}

bool ReaderSession::samePosition(
    const SemanticPosition& a,
    const SemanticPosition& b
) {
    return a.book_id == b.book_id &&
           a.section_id == b.section_id &&
           a.text_offset == b.text_offset;
}

std::optional<PageResult> ReaderSession::layoutAt(
    const SemanticPosition& anchor
) {
    request_.anchor = anchor;
    return engine_.layoutPage(request_);
}

void ReaderSession::applyHistoryAnchors(PageResult& page) const {
    if (history_.empty()) {
        page.previous_anchor.reset();
    } else {
        page.previous_anchor = history_.back();
    }
}

void ReaderSession::prefetchNext() {
    next_page_.reset();

    if (!current_page_.has_value() ||
        !current_page_->next_anchor.has_value()) {
        return;
    }

    auto page = layoutAt(*current_page_->next_anchor);
    if (!page.has_value()) {
        return;
    }

    next_page_ = std::move(page);
}

void ReaderSession::prefetchPrevious() {
    previous_page_.reset();

    if (history_.empty()) {
        return;
    }

    auto page = layoutAt(history_.back());
    if (!page.has_value()) {
        return;
    }

    if (history_.size() >= 2) {
        page->previous_anchor = history_[history_.size() - 2];
    } else {
        page->previous_anchor.reset();
    }

    previous_page_ = std::move(page);
}

ReaderSessionStatus ReaderSession::open(
    const LayoutRequest& request
) {
    request_ = request;
    history_.clear();
    previous_page_.reset();
    next_page_.reset();

    current_page_ = engine_.layoutPage(request_);
    if (!current_page_.has_value()) {
        status_ = ReaderSessionStatus::LayoutFailed;
        return status_;
    }

    applyHistoryAnchors(*current_page_);
    prefetchNext();

    // Prefetch changes request_.anchor internally; restore the authoritative
    // current-page anchor after cache preparation.
    request_.anchor = current_page_->first_position;

    status_ = ReaderSessionStatus::Ready;
    return status_;
}

ReaderSessionStatus ReaderSession::next() {
    if (!current_page_.has_value()) {
        status_ = ReaderSessionStatus::Closed;
        return status_;
    }

    if (!current_page_->next_anchor.has_value()) {
        status_ = ReaderSessionStatus::EndOfBook;
        return status_;
    }

    const auto target = *current_page_->next_anchor;
    const auto old_current = *current_page_;

    std::optional<PageResult> next;
    if (next_page_.has_value() &&
        samePosition(next_page_->first_position, target)) {
        next = std::move(next_page_);
    } else {
        next = layoutAt(target);
    }

    if (!next.has_value()) {
        status_ = ReaderSessionStatus::LayoutFailed;
        return status_;
    }

    history_.push_back(old_current.first_position);
    previous_page_ = old_current;
    current_page_ = std::move(next);

    applyHistoryAnchors(*current_page_);
    prefetchNext();
    request_.anchor = current_page_->first_position;

    status_ = ReaderSessionStatus::Ready;
    return status_;
}

ReaderSessionStatus ReaderSession::previous() {
    if (!current_page_.has_value()) {
        status_ = ReaderSessionStatus::Closed;
        return status_;
    }

    if (history_.empty()) {
        status_ = ReaderSessionStatus::BeginningOfBook;
        return status_;
    }

    const auto target = history_.back();
    history_.pop_back();

    const auto old_current = *current_page_;
    std::optional<PageResult> previous;

    if (previous_page_.has_value() &&
        samePosition(previous_page_->first_position, target)) {
        previous = std::move(previous_page_);
    } else {
        previous = layoutAt(target);
    }

    if (!previous.has_value()) {
        history_.push_back(target);
        status_ = ReaderSessionStatus::LayoutFailed;
        return status_;
    }

    next_page_ = old_current;
    current_page_ = std::move(previous);
    applyHistoryAnchors(*current_page_);

    // Rebuild the older adjacent cache from deterministic history.
    prefetchPrevious();
    request_.anchor = current_page_->first_position;

    status_ = ReaderSessionStatus::Ready;
    return status_;
}

void ReaderSession::invalidateLayout(
    const TypographySettings& typography,
    const Viewport& viewport
) {
    if (!current_page_.has_value()) {
        request_.typography = typography;
        request_.viewport = viewport;
        return;
    }

    const auto anchor = current_page_->first_position;

    request_.typography = typography;
    request_.viewport = viewport;
    request_.anchor = anchor;

    history_.clear();
    previous_page_.reset();
    next_page_.reset();

    current_page_ = engine_.layoutPage(request_);
    if (!current_page_.has_value()) {
        status_ = ReaderSessionStatus::LayoutFailed;
        return;
    }

    current_page_->previous_anchor.reset();
    prefetchNext();
    request_.anchor = current_page_->first_position;
    status_ = ReaderSessionStatus::Ready;
}

bool ReaderSession::isOpen() const {
    return current_page_.has_value();
}

const std::optional<PageResult>& ReaderSession::currentPage() const {
    return current_page_;
}

ReaderSessionStatus ReaderSession::status() const {
    return status_;
}

} // namespace enku
