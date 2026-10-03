#include "enku/runtime/book_availability_reconciler.hpp"
#include "enku/storage/book_fingerprint.hpp"

namespace enku {

BookAvailabilityReconciler::BookAvailabilityReconciler(
    AppState& app_state,
    CborLibraryService& library,
    BookFileStore& files
)
    : app_state_(app_state),
      library_(library),
      files_(files) {}

void BookAvailabilityReconciler::reconcile() {
    auto& unavailable =
        app_state_.library.unavailable_books;
    unavailable.clear();

    const auto& records = library_.records();

    if (app_state_.storage.removable !=
        RemovableStorageStatus::Ready) {
        for (const auto& record : records) {
            unavailable.push_back(record.book_id);
        }
        return;
    }

    for (const auto& record : records) {
        const auto actual =
            fingerprintStoredBook(
                files_,
                record.source_path
            );

        if (!actual.ok() ||
            actual.file_size != record.file_size ||
            actual.fingerprint != record.fingerprint) {
            unavailable.push_back(record.book_id);
        }
    }
}

} // namespace enku
