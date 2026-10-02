#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "../core/app_state.hpp"
#include "../storage/book_file_store.hpp"
#include "../storage/staged_book_import_service.hpp"

namespace enku {

enum class WebUploadStatus : std::uint8_t {
    Ok,
    Busy,
    InvalidFilename,
    EmptyPayload,
    PayloadTooLarge,
    StageWriteFailed,
    ImportFailed,
};

struct WebUploadResult {
    WebUploadStatus status{WebUploadStatus::ImportFailed};
    StagedImportStatus import_status{
        StagedImportStatus::StageReadFailed
    };
    BookId book_id;

    bool ok() const {
        return status == WebUploadStatus::Ok;
    }
};

class WebUploadIngress {
public:
    static constexpr std::size_t kDefaultMaxPayloadBytes =
        8U * 1024U * 1024U;

    WebUploadIngress(
        AppState& app_state,
        BookFileStore& files,
        StagedBookImportService& staged_import,
        std::size_t max_payload_bytes =
            kDefaultMaxPayloadBytes
    );

    WebUploadResult upload(
        std::string_view source_filename,
        std::string_view bytes,
        std::uint64_t added_order
    );

private:
    AppState& app_state_;
    BookFileStore& files_;
    StagedBookImportService& staged_import_;
    std::size_t max_payload_bytes_;

    static bool validFilename(
        std::string_view filename
    );
};

} // namespace enku
