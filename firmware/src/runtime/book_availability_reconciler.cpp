#include "enku/runtime/book_availability_reconciler.hpp"
#include "enku/storage/book_fingerprint.hpp"

#include <algorithm>

namespace enku {

BookAvailabilityReconciler::BookAvailabilityReconciler(
    AppState& app_state,
    CborLibraryService& library,
    BookFileStore& files
)
    : app_state_(app_state),
      library_(library),
      files_(files) {}

void BookAvailabilityReconciler::markAllUnavailable() {
    auto& unavailable =
        app_state_.library.unavailable_books;
    unavailable.clear();
    app_state_.library.availability_pending_books.clear();

    for (const auto& record : library_.records()) {
        unavailable.push_back(record.book_id);
    }
}

void BookAvailabilityReconciler::reconcile() {
    incremental_active_ = false;
    pending_records_.clear();
    next_record_ = 0;
    app_state_.library.availability_check_active = false;
    app_state_.library.availability_checked = 0;
    app_state_.library.availability_total = 0;

    markAllUnavailable();

    if (app_state_.storage.removable !=
        RemovableStorageStatus::Ready) {
        return;
    }

    const auto records = library_.records();
    auto& unavailable =
        app_state_.library.unavailable_books;

    for (const auto& record : records) {
        const auto actual =
            fingerprintStoredBook(
                files_,
                record.source_path
            );

        if (actual.ok() &&
            actual.file_size == record.file_size &&
            actual.fingerprint == record.fingerprint) {
            unavailable.erase(
                std::remove(
                    unavailable.begin(),
                    unavailable.end(),
                    record.book_id
                ),
                unavailable.end()
            );
        }
    }
}

void BookAvailabilityReconciler::beginIncremental() {
    markAllUnavailable();
    pending_records_ = library_.records();
    next_record_ = 0;
    app_state_.library.availability_pending_books.clear();
    for (const auto& record : pending_records_) {
        app_state_.library.availability_pending_books.push_back(
            record.book_id
        );
    }
    app_state_.library.availability_checked = 0;
    app_state_.library.availability_total =
        static_cast<std::uint32_t>(pending_records_.size());
    incremental_active_ =
        app_state_.storage.removable ==
            RemovableStorageStatus::Ready &&
        !pending_records_.empty();
    app_state_.library.availability_check_active =
        incremental_active_;
}

bool BookAvailabilityReconciler::step(
    std::size_t max_records
) {
    if (!incremental_active_) {
        return true;
    }

    if (app_state_.storage.removable !=
        RemovableStorageStatus::Ready) {
        incremental_active_ = false;
        pending_records_.clear();
        next_record_ = 0;
        app_state_.library.availability_check_active = false;
        app_state_.library.availability_checked = 0;
        app_state_.library.availability_total = 0;
        markAllUnavailable();
        return true;
    }

    auto& unavailable =
        app_state_.library.unavailable_books;
    std::size_t processed = 0;

    while (next_record_ < pending_records_.size() &&
           processed < max_records) {
        const auto& record =
            pending_records_[next_record_++];

        auto& pending =
            app_state_.library.availability_pending_books;
        pending.erase(
            std::remove(
                pending.begin(),
                pending.end(),
                record.book_id
            ),
            pending.end()
        );

        const auto actual =
            fingerprintStoredBook(
                files_,
                record.source_path
            );

        if (actual.ok() &&
            actual.file_size == record.file_size &&
            actual.fingerprint == record.fingerprint) {
            unavailable.erase(
                std::remove(
                    unavailable.begin(),
                    unavailable.end(),
                    record.book_id
                ),
                unavailable.end()
            );
        }

        ++processed;
        app_state_.library.availability_checked =
            static_cast<std::uint32_t>(next_record_);
    }

    if (next_record_ >= pending_records_.size()) {
        incremental_active_ = false;
        pending_records_.clear();
        next_record_ = 0;
        app_state_.library.availability_check_active = false;
        app_state_.library.availability_pending_books.clear();
        return true;
    }

    return false;
}

bool BookAvailabilityReconciler::active() const {
    return incremental_active_;
}

} // namespace enku
