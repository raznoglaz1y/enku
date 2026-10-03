#include "enku/runtime/book_availability_reconciler.hpp"

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
        std::uint64_t actual_size = 0;
        const auto status =
            files_.size(record.source_path, actual_size);

        if (status != BookFileStatus::Ok ||
            actual_size != record.file_size) {
            unavailable.push_back(record.book_id);
        }
    }
}

} // namespace enku
