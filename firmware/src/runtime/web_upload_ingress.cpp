#include "enku/runtime/web_upload_ingress.hpp"

#include <string>

namespace enku {
namespace {

constexpr const char* kWebUploadStagePath =
    "/system/incoming/web-upload.tmp";

class ImportActivityGuard {
public:
    explicit ImportActivityGuard(AppState& app_state)
        : app_state_(app_state) {
        app_state_.import_active = true;
    }

    ~ImportActivityGuard() {
        app_state_.import_active = false;
    }

private:
    AppState& app_state_;
};

} // namespace

WebUploadIngress::WebUploadIngress(
    AppState& app_state,
    BookFileStore& files,
    StagedBookImportService& staged_import,
    std::size_t max_payload_bytes
)
    : app_state_(app_state),
      files_(files),
      staged_import_(staged_import),
      max_payload_bytes_(max_payload_bytes) {}

bool WebUploadIngress::validFilename(
    std::string_view filename
) {
    if (filename.empty() ||
        filename == "." ||
        filename == "..") {
        return false;
    }

    for (const unsigned char ch : filename) {
        if (ch == '/' ||
            ch == '\\' ||
            ch == '\0' ||
            ch < 0x20U ||
            ch == 0x7FU) {
            return false;
        }
    }

    return true;
}

WebUploadResult WebUploadIngress::upload(
    std::string_view source_filename,
    std::string_view bytes,
    std::uint64_t added_order
) {
    WebUploadResult result;

    if (app_state_.import_active) {
        result.status = WebUploadStatus::Busy;
        return result;
    }

    if (!validFilename(source_filename)) {
        result.status =
            WebUploadStatus::InvalidFilename;
        return result;
    }

    if (bytes.empty()) {
        result.status =
            WebUploadStatus::EmptyPayload;
        return result;
    }

    if (bytes.size() > max_payload_bytes_) {
        result.status =
            WebUploadStatus::PayloadTooLarge;
        return result;
    }

    ImportActivityGuard activity(app_state_);

    const std::string payload(bytes);

    if (files_.write(
            kWebUploadStagePath,
            payload
        ) != BookFileStatus::Ok) {
        result.status =
            WebUploadStatus::StageWriteFailed;
        return result;
    }

    const auto imported =
        staged_import_.import(
            kWebUploadStagePath,
            std::string(source_filename),
            added_order
        );

    result.import_status = imported.status;
    result.book_id = imported.book_id;

    if (!imported.ok()) {
        // The staged importer intentionally preserves failed stages for
        // storage-level recovery. Web uploads are single-shot requests, so
        // remove the ingress temp file best-effort to avoid blocking the next
        // upload with stale bytes.
        files_.remove(kWebUploadStagePath);
        result.status =
            WebUploadStatus::ImportFailed;
        return result;
    }

    result.status = WebUploadStatus::Ok;
    return result;
}

} // namespace enku
