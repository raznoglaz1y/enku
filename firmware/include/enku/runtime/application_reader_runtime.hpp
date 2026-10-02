#pragma once

#include "../reader/book_loader.hpp"
#include "../render/reader_page_renderer.hpp"
#include "../services/services.hpp"
#include "application_storage_runtime.hpp"
#include "boot_restore.hpp"
#include "library_runtime.hpp"
#include "reader_runtime.hpp"

namespace enku {

class ApplicationReaderRuntime {
public:
    ApplicationReaderRuntime(
        ApplicationStorageRuntime& storage,
        RefreshService& refresh,
        const TextMeasurer& measurer,
        ReaderPageRenderer& renderer,
        TypographySettings typography,
        Viewport viewport
    );

    ReaderBookLoader& loader();
    ReaderRuntimeController& reader();
    LibraryRuntimeController& library();
    BootRestoreCoordinator& bootRestore();

private:
    ApplicationStorageRuntime& storage_;

    ReaderBookLoader loader_;
    ReaderRuntimeController reader_;
    LibraryRuntimeController library_;
    BootRestoreCoordinator boot_restore_;
};

} // namespace enku
