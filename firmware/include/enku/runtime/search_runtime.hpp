#pragma once

#include <cstdint>

#include "../core/input.hpp"
#include "../render/search_renderer.hpp"
#include "../reader/document_search.hpp"
#include "../services/services.hpp"
#include "application_reader_runtime.hpp"
#include "keyboard_runtime.hpp"
#include "application_storage_runtime.hpp"

namespace enku {

enum class SearchRuntimeResult : std::uint8_t {
    Ignored,
    Applied,
    Failed,
};

class SearchRuntime {
public:
    SearchRuntime(
        ApplicationStorageRuntime& storage,
        ApplicationReaderRuntime& reader,
        SearchRenderer* renderer = nullptr,
        RefreshService* refresh = nullptr
    );

    SearchRuntimeResult openFromReader();
    SearchRuntimeResult handle(LogicalAction action);
    SearchRuntimeResult submitQuery(std::string query);

private:
    ApplicationStorageRuntime& storage_;
    ApplicationReaderRuntime& reader_;
    SearchRenderer* renderer_{nullptr};
    RefreshService* refresh_{nullptr};
    KeyboardRuntime keyboard_;
    DocumentSearch document_search_;

    SearchRuntimeResult cancel();
    SearchRuntimeResult executeSearch();
    SearchRuntimeResult populateWindow(
        std::uint32_t window_start
    );
    SearchRuntimeResult handleKeyboard(LogicalAction action);
    SearchRuntimeResult render();
    SearchRuntimeResult refreshCurrentFrame();
};

} // namespace enku
