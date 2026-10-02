#include "enku/runtime/application_storage_runtime.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"

#include <cassert>
#include <filesystem>

using namespace enku;

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-application-storage-runtime-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);

    ApplicationStorageRuntime runtime(
        state_files,
        book_files
    );

    const auto startup =
        runtime.storageStartup().run();

    assert(startup.ready());
    assert(
        runtime.appState().screen ==
        Screen::Library
    );

    const std::string stage =
        "/system/tmp/runtime-book.txt";

    assert(
        book_files.write(
            stage,
            "Runtime composed import."
        ) == BookFileStatus::Ok
    );

    const auto imported =
        runtime.stagedImport().import(
            stage,
            "Runtime Book.txt",
            1
        );

    assert(imported.ok());
    assert(
        runtime.library().get(
            imported.book_id
        ).has_value()
    );

    const auto book =
        runtime.library().get(
            imported.book_id
        );
    assert(book.has_value());

    std::string bytes;
    assert(
        runtime.bookSource().readSource(
            *book,
            bytes
        ) == BookSourceStatus::Ok
    );
    assert(bytes == "Runtime composed import.");

    assert(
        runtime.deleteService().remove(
            imported.book_id
        ) == BookDeleteStatus::Ok
    );
    assert(
        !runtime.library().get(
            imported.book_id
        ).has_value()
    );

    std::filesystem::remove_all(root, ec);
    return 0;
}
