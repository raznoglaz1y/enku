#pragma once

#include <cstddef>
#include <vector>

#include "../core/app_state.hpp"
#include "../storage/book_file_store.hpp"
#include "../storage/cbor_library_service.hpp"

namespace enku {

class BookAvailabilityReconciler {
public:
    BookAvailabilityReconciler(
        AppState& app_state,
        CborLibraryService& library,
        BookFileStore& files
    );

    // Synchronous compatibility path used at boot and by focused tests.
    void reconcile();

    // Remount path: fail closed immediately, then verify a bounded number of
    // records per device poll so a large library cannot stall the UI.
    void beginIncremental();
    bool step(std::size_t max_records = 1U);
    bool active() const;

private:
    void markAllUnavailable();

    AppState& app_state_;
    CborLibraryService& library_;
    BookFileStore& files_;
    std::vector<BookRecord> pending_records_;
    std::size_t next_record_{0};
    bool incremental_active_{false};
};

} // namespace enku
