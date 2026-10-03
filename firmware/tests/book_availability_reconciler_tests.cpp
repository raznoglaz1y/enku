#include "enku/runtime/book_availability_reconciler.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/book_fingerprint.hpp"

#include <cassert>
#include <filesystem>

using namespace enku;

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-book-availability-reconciler-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);
    CborLibraryService library(state_files);

    BookRecord first;
    first.book_id = "first";
    first.format = BookFormat::Txt;
    first.source_path = "/books/first.txt";
    first.source_filename = "first.txt";
    first.file_size = 5;
    first.fingerprint = fingerprintBookBytes("12345");

    BookRecord second;
    second.book_id = "second";
    second.format = BookFormat::Txt;
    second.source_path = "/books/second.txt";
    second.source_filename = "second.txt";
    second.file_size = 6;
    second.fingerprint = fingerprintBookBytes("123456");

    assert(library.upsert(first) == LibraryStatus::Ok);
    assert(library.upsert(second) == LibraryStatus::Ok);

    AppState app;
    BookAvailabilityReconciler reconciler(
        app,
        library,
        book_files
    );

    app.storage.removable =
        RemovableStorageStatus::Unavailable;
    reconciler.reconcile();

    assert(!app.library.bookAvailable("first"));
    assert(!app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.size() == 2U);

    app.storage.removable =
        RemovableStorageStatus::Ready;

    // A returning card is not enough by itself: a missing source and a
    // same-path replacement with changed size both stay unavailable.
    assert(
        book_files.write(
            "/books/first.txt",
            "changed"
        ) == BookFileStatus::Ok
    );
    reconciler.reconcile();

    assert(!app.library.bookAvailable("first"));
    assert(!app.library.bookAvailable("second"));

    // Matching size alone is not identity. A replacement card can contain a
    // different file at the same path with exactly the same byte length.
    assert(
        book_files.write(
            "/books/first.txt",
            "abcde"
        ) == BookFileStatus::Ok
    );
    reconciler.reconcile();
    assert(!app.library.bookAvailable("first"));

    // Restore the original first source. Only that record becomes available.
    assert(
        book_files.write(
            "/books/first.txt",
            "12345"
        ) == BookFileStatus::Ok
    );
    reconciler.reconcile();

    assert(app.library.bookAvailable("first"));
    assert(!app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.size() == 1U);

    // Once both matching sources are back, stale unavailable state is cleared.
    assert(
        book_files.write(
            "/books/second.txt",
            "123456"
        ) == BookFileStatus::Ok
    );
    reconciler.reconcile();

    assert(app.library.bookAvailable("first"));
    assert(app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.empty());

    // Repeated reconciliation is idempotent and cannot accumulate duplicates.
    reconciler.reconcile();
    assert(app.library.unavailable_books.empty());

    // A second removal invalidates recovered books again.
    app.storage.removable =
        RemovableStorageStatus::Unavailable;
    reconciler.reconcile();
    assert(!app.library.bookAvailable("first"));
    assert(!app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.size() == 2U);

    // Directory setup failure is non-readable media too.
    app.storage.removable =
        RemovableStorageStatus::SetupError;
    reconciler.reconcile();
    assert(!app.library.bookAvailable("first"));
    assert(!app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.size() == 2U);

    // A later successful retry derives availability from real files,
    // not stale state from the previous card lifecycle.
    app.storage.removable =
        RemovableStorageStatus::Ready;
    reconciler.reconcile();
    assert(app.library.bookAvailable("first"));
    assert(app.library.bookAvailable("second"));
    assert(app.library.unavailable_books.empty());

    // The complete remove/reinsert cycle remains repeatable.
    app.storage.removable =
        RemovableStorageStatus::Unavailable;
    reconciler.reconcile();
    app.storage.removable =
        RemovableStorageStatus::Ready;
    reconciler.reconcile();
    assert(app.library.unavailable_books.empty());

    std::filesystem::remove_all(root, ec);
    return 0;
}
