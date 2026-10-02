#include "enku/runtime/web_upload_ingress.hpp"

#include <string>

namespace enku {
namespace {

constexpr const char* kWebUploadStagePath =
    "/system/incoming/web-upload.tmp";

} // namespace

WebUploadIngress::WebUploadIngress(
    BookFileStore& files,
    StagedBookImportService& staged_import,
    std::size_t max_payload_bytes
)
    : files_(files),
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

void WebUploadIngress::resetSession(
    bool remove_stage
) {
    if (remove_stage) {
        files_.remove(kWebUploadStagePath);
    }

    session_active_.store(false);
    source_filename_.clear();
    expected_bytes_ = 0;
    received_bytes_ = 0;
}

WebUploadResult WebUploadIngress::begin(
    std::string_view source_filename,
    std::size_t content_length
) {
    WebUploadResult result;

    if (session_active_.load()) {
        result.status = WebUploadStatus::Busy;
        return result;
    }

    if (!validFilename(source_filename)) {
        result.status =
            WebUploadStatus::InvalidFilename;
        return result;
    }

    if (content_length == 0) {
        result.status =
            WebUploadStatus::EmptyPayload;
        return result;
    }

    if (content_length > max_payload_bytes_) {
        result.status =
            WebUploadStatus::PayloadTooLarge;
        return result;
    }

    if (files_.write(
            kWebUploadStagePath,
            {}
        ) != BookFileStatus::Ok) {
        result.status =
            WebUploadStatus::StageWriteFailed;
        return result;
    }

    source_filename_ =
        std::string(source_filename);
    expected_bytes_ = content_length;
    received_bytes_ = 0;
    session_active_.store(true);

    result.status = WebUploadStatus::Ok;
    return result;
}

WebUploadResult WebUploadIngress::appendChunk(
    std::string_view bytes
) {
    WebUploadResult result;

    if (!session_active_.load()) {
        result.status =
            WebUploadStatus::NoActiveUpload;
        return result;
    }

    if (bytes.empty()) {
        result.status = WebUploadStatus::Ok;
        return result;
    }

    if (received_bytes_ + bytes.size() >
        expected_bytes_) {
        resetSession(true);
        result.status =
            WebUploadStatus::PayloadLengthMismatch;
        return result;
    }

    if (files_.append(
            kWebUploadStagePath,
            std::string(bytes)
        ) != BookFileStatus::Ok) {
        resetSession(true);
        result.status =
            WebUploadStatus::StageWriteFailed;
        return result;
    }

    received_bytes_ += bytes.size();
    result.status = WebUploadStatus::Ok;
    return result;
}

WebUploadResult WebUploadIngress::finish(
    std::uint64_t added_order
) {
    WebUploadResult result;

    if (!session_active_.load()) {
        result.status =
            WebUploadStatus::NoActiveUpload;
        return result;
    }

    if (received_bytes_ != expected_bytes_) {
        resetSession(true);
        result.status =
            WebUploadStatus::PayloadLengthMismatch;
        return result;
    }

    const auto source_filename =
        source_filename_;

    const auto imported =
        staged_import_.import(
            kWebUploadStagePath,
            source_filename,
            added_order
        );

    result.import_status = imported.status;
    result.book_id = imported.book_id;

    if (!imported.ok()) {
        resetSession(true);
        result.status =
            WebUploadStatus::ImportFailed;
        return result;
    }

    resetSession(false);
    result.status = WebUploadStatus::Ok;
    return result;
}

void WebUploadIngress::cancel() {
    if (!session_active_.load()) {
        return;
    }

    resetSession(true);
}

bool WebUploadIngress::active() const {
    return session_active_.load();
}

WebUploadResult WebUploadIngress::upload(
    std::string_view source_filename,
    std::string_view bytes,
    std::uint64_t added_order
) {
    auto result =
        begin(
            source_filename,
            bytes.size()
        );

    if (!result.ok()) {
        return result;
    }

    result = appendChunk(bytes);
    if (!result.ok()) {
        return result;
    }

    return finish(added_order);
}

} // namespace enku
