#include "enku/runtime/application_reader_runtime.hpp"

namespace enku {

ApplicationReaderRuntime::ApplicationReaderRuntime(
    ApplicationStorageRuntime& storage,
    RefreshService& refresh,
    const TextMeasurer& measurer,
    ReaderPageRenderer& reader_renderer,
    LibraryPageRenderer& library_renderer,
    TypographySettings typography,
    Viewport viewport
)
    : storage_(storage),
      loader_(
          storage_.library(),
          storage_.bookSource(),
          measurer
      ),
      reader_(
          storage_.appState(),
          loader_,
          refresh,
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          typography,
          viewport,
          &reader_renderer
      ),
      library_(
          storage_.appState(),
          storage_.library(),
          reader_,
          storage_.stagedImport(),
          storage_.deleteService(),
          storage_.settingsRuntime(),
          refresh,
          &library_renderer
      ),
      boot_restore_(
          storage_.appState(),
          storage_.storageStartup(),
          storage_.appContext(),
          storage_.bootLoop(),
          reader_,
          storage_.library()
      ) {}

ReaderBookLoader&
ApplicationReaderRuntime::loader() {
    return loader_;
}

ReaderRuntimeController&
ApplicationReaderRuntime::reader() {
    return reader_;
}

LibraryRuntimeController&
ApplicationReaderRuntime::library() {
    return library_;
}

BootRestoreCoordinator&
ApplicationReaderRuntime::bootRestore() {
    return boot_restore_;
}

} // namespace enku
