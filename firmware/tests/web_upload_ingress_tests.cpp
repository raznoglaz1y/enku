#include "enku/runtime/web_upload_ingress.hpp"
#include "enku/storage/book_import_service.hpp"
#include "enku/storage/cbor_library_service.hpp"
#include "enku/storage/posix_book_file_store.hpp"
#include "enku/storage/posix_state_file_store.hpp"
#include "enku/storage/staged_book_import_service.hpp"

#include <cassert>
#include <filesystem>
#include <string>

using namespace enku;

int main() {
    const auto root =
        std::filesystem::temp_directory_path() /
        "enku-web-upload-ingress-test";

    std::error_code ec;
    std::filesystem::remove_all(root, ec);

    PosixStateFileStore state_files(root);
    PosixBookFileStore book_files(root);

    CborLibraryService library(state_files);
    assert(library.load() == LibraryStatus::Ok);

    BookImportService importer(library);
    StagedBookImportService staged(
        book_files,
        importer
    );

    AppState app;
    WebUploadIngress ingress(
        app,
        book_files,
        staged,
        1024
    );

    assert(
        ingress.upload(
            "../escape.txt",
            "hello",
            1
        ).status ==
        WebUploadStatus::InvalidFilename
    );

    assert(
        ingress.upload(
            "empty.txt",
            "",
            1
        ).status ==
        WebUploadStatus::EmptyPayload
    );

    assert(
        ingress.upload(
            "large.txt",
            std::string(1025, 'x'),
            1
        ).status ==
        WebUploadStatus::PayloadTooLarge
    );

    app.import_active = true;
    assert(
        ingress.upload(
            "busy.txt",
            "busy",
            1
        ).status ==
        WebUploadStatus::Busy
    );
    app.import_active = false;

    const auto uploaded =
        ingress.upload(
            "Hello ENKU.txt",
            "Hello from the ENKU web uploader.\n",
            42
        );

    assert(uploaded.ok());
    assert(!app.import_active);
    assert(!uploaded.book_id.empty());

    const auto record =
        library.get(uploaded.book_id);

    assert(record.has_value());
    assert(
        record->source_filename ==
        "Hello ENKU.txt"
    );
    assert(record->added_order == 42);

    std::string stored;
    assert(
        book_files.read(
            record->source_path,
            stored
        ) == BookFileStatus::Ok
    );
    assert(
        stored ==
        "Hello from the ENKU web uploader.\n"
    );

    std::string stale_stage;
    assert(
        book_files.read(
            "/system/incoming/web-upload.tmp",
            stale_stage
        ) == BookFileStatus::NotFound
    );

    const auto duplicate =
        ingress.upload(
            "duplicate.txt",
            "Hello from the ENKU web uploader.\n",
            43
        );

    assert(
        duplicate.status ==
        WebUploadStatus::ImportFailed
    );
    assert(
        duplicate.import_status ==
        StagedImportStatus::Duplicate
    );
    assert(!app.import_active);

    assert(
        book_files.read(
            "/system/incoming/web-upload.tmp",
            stale_stage
        ) == BookFileStatus::NotFound
    );

    std::filesystem::remove_all(root, ec);
    return 0;
}
