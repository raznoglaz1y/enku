#pragma once

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

    void reconcile();

private:
    AppState& app_state_;
    CborLibraryService& library_;
    BookFileStore& files_;
};

} // namespace enku
