#include "enku/platform/esp_idf/esp_idf_web_upload_server.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>

#include "esp_log.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_WEB";

constexpr const char* kIndexHtml = R"html(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ENKU Upload</title>
<style>
body{font-family:system-ui,sans-serif;max-width:520px;margin:48px auto;padding:0 20px;color:#111}
h1{font-size:28px;margin-bottom:8px}
p{line-height:1.5;color:#555}
input,button{font:inherit}
input{display:block;margin:24px 0 16px;width:100%}
button{padding:12px 18px;border:1px solid #111;background:#111;color:#fff;border-radius:8px}
button:disabled{opacity:.45}
#status{margin-top:20px;white-space:pre-wrap}
</style>
</head>
<body>
<h1>ENKU</h1>
<p>Upload a TXT book to this reader over the local network.</p>
<input id="file" type="file" accept=".txt,text/plain">
<button id="upload">Upload</button>
<div id="status"></div>
<script>
const file=document.getElementById('file');
const button=document.getElementById('upload');
const status=document.getElementById('status');
button.onclick=async()=>{
  const f=file.files[0];
  if(!f){status.textContent='Choose a TXT file first.';return;}
  button.disabled=true;
  status.textContent='Uploading…';
  try{
    const r=await fetch('/api/upload',{
      method:'POST',
      headers:{'X-ENKU-Filename':f.name,'Content-Type':'application/octet-stream'},
      body:f
    });
    const t=await r.text();
    status.textContent=r.ok?'Uploaded: '+f.name:t;
  }catch(e){
    status.textContent='Upload failed: '+e;
  }finally{
    button.disabled=false;
  }
};
</script>
</body>
</html>
)html";

void sendJson(
    httpd_req_t* request,
    const char* status,
    const std::string& body
) {
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(
        request,
        "application/json"
    );
    httpd_resp_send(
        request,
        body.c_str(),
        body.size()
    );
}

const char* uploadStatusName(
    WebUploadStatus status
) {
    switch (status) {
        case WebUploadStatus::Ok:
            return "ok";
        case WebUploadStatus::Busy:
            return "busy";
        case WebUploadStatus::InvalidFilename:
            return "invalid_filename";
        case WebUploadStatus::EmptyPayload:
            return "empty_payload";
        case WebUploadStatus::PayloadTooLarge:
            return "payload_too_large";
        case WebUploadStatus::NoActiveUpload:
            return "no_active_upload";
        case WebUploadStatus::PayloadLengthMismatch:
            return "payload_length_mismatch";
        case WebUploadStatus::StageWriteFailed:
            return "stage_write_failed";
        case WebUploadStatus::ImportFailed:
            return "import_failed";
    }
    return "unknown";
}

} // namespace

EspIdfWebUploadServer::EspIdfWebUploadServer(
    WebUploadIngress& ingress,
    LibraryService& library
)
    : ingress_(ingress),
      library_(library) {}

EspIdfWebUploadServer::~EspIdfWebUploadServer() {
    stop();
}

bool EspIdfWebUploadServer::running() const {
    return server_ != nullptr;
}

bool EspIdfWebUploadServer::takeUploadCompleted() {
    return upload_completed_.exchange(false);
}

bool EspIdfWebUploadServer::sync(bool online) {
    if (online) {
        return running() || start();
    }

    stop();
    return true;
}

bool EspIdfWebUploadServer::start() {
    if (running()) {
        return true;
    }

    httpd_config_t config =
        HTTPD_DEFAULT_CONFIG();

    config.max_uri_handlers = 4;
    config.stack_size = 6144;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    if (httpd_start(
            &server_,
            &config
        ) != ESP_OK) {
        server_ = nullptr;
        ESP_LOGE(
            kTag,
            "Failed to start HTTP server"
        );
        return false;
    }

    httpd_uri_t index = {};
    index.uri = "/";
    index.method = HTTP_GET;
    index.handler =
        &EspIdfWebUploadServer::handleIndex;
    index.user_ctx = this;

    httpd_uri_t health = {};
    health.uri = "/api/health";
    health.method = HTTP_GET;
    health.handler =
        &EspIdfWebUploadServer::handleHealth;
    health.user_ctx = this;

    httpd_uri_t upload_uri = {};
    upload_uri.uri = "/api/upload";
    upload_uri.method = HTTP_POST;
    upload_uri.handler =
        &EspIdfWebUploadServer::handleUpload;
    upload_uri.user_ctx = this;

    const bool registered =
        httpd_register_uri_handler(
            server_,
            &index
        ) == ESP_OK &&
        httpd_register_uri_handler(
            server_,
            &health
        ) == ESP_OK &&
        httpd_register_uri_handler(
            server_,
            &upload_uri
        ) == ESP_OK;

    if (!registered) {
        ESP_LOGE(
            kTag,
            "Failed to register HTTP handlers"
        );
        stop();
        return false;
    }

    ESP_LOGI(
        kTag,
        "Local web uploader started on port %u",
        static_cast<unsigned>(config.server_port)
    );

    return true;
}

void EspIdfWebUploadServer::stop() {
    if (!running()) {
        return;
    }

    ingress_.cancel();
    httpd_stop(server_);
    server_ = nullptr;

    ESP_LOGI(
        kTag,
        "Local web uploader stopped"
    );
}

std::uint64_t
EspIdfWebUploadServer::nextAddedOrder() const {
    LibraryQuery query;
    query.mode = LibraryQueryMode::Browse;
    query.filter = LibraryFilter::All;
    query.sort = LibrarySort::RecentlyAdded;
    query.direction = SortDirection::Descending;
    query.offset = 0;
    query.limit = 1;

    LibraryPage page;
    if (library_.query(query, page) !=
            LibraryStatus::Ok ||
        page.items.empty()) {
        return 1;
    }

    return page.items.front().added_order + 1U;
}

esp_err_t EspIdfWebUploadServer::handleIndex(
    httpd_req_t* request
) {
    httpd_resp_set_type(
        request,
        "text/html; charset=utf-8"
    );
    return httpd_resp_send(
        request,
        kIndexHtml,
        HTTPD_RESP_USE_STRLEN
    );
}

esp_err_t EspIdfWebUploadServer::handleHealth(
    httpd_req_t* request
) {
    httpd_resp_set_type(
        request,
        "application/json"
    );
    return httpd_resp_sendstr(
        request,
        "{\"status\":\"ok\",\"service\":\"enku-upload\"}"
    );
}

esp_err_t EspIdfWebUploadServer::handleUpload(
    httpd_req_t* request
) {
    auto* self =
        static_cast<EspIdfWebUploadServer*>(
            request->user_ctx
        );

    if (self == nullptr) {
        httpd_resp_send_err(
            request,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "server context unavailable"
        );
        return ESP_FAIL;
    }

    return self->upload(request);
}

esp_err_t EspIdfWebUploadServer::upload(
    httpd_req_t* request
) {
    const auto filename_length =
        httpd_req_get_hdr_value_len(
            request,
            "X-ENKU-Filename"
        );

    if (filename_length == 0 ||
        filename_length > 255) {
        sendJson(
            request,
            "400 Bad Request",
            "{\"error\":\"missing_or_invalid_filename\"}"
        );
        return ESP_OK;
    }

    std::string filename(
        filename_length + 1U,
        '\0'
    );

    if (httpd_req_get_hdr_value_str(
            request,
            "X-ENKU-Filename",
            filename.data(),
            filename.size()
        ) != ESP_OK) {
        sendJson(
            request,
            "400 Bad Request",
            "{\"error\":\"invalid_filename_header\"}"
        );
        return ESP_OK;
    }

    filename.resize(filename_length);

    const auto content_length =
        static_cast<std::size_t>(
            request->content_len
        );

    auto result =
        ingress_.begin(
            filename,
            content_length
        );

    if (!result.ok()) {
        const char* response_status =
            result.status ==
                WebUploadStatus::PayloadTooLarge
                ? "413 Payload Too Large"
                : result.status ==
                    WebUploadStatus::Busy
                    ? "409 Conflict"
                    : "400 Bad Request";

        sendJson(
            request,
            response_status,
            std::string("{\"error\":\"") +
                uploadStatusName(result.status) +
                "\"}"
        );
        return ESP_OK;
    }

    std::array<char, 4096> buffer = {};
    std::size_t remaining =
        content_length;

    while (remaining > 0) {
        const auto requested =
            std::min<std::size_t>(
                buffer.size(),
                remaining
            );

        const int received =
            httpd_req_recv(
                request,
                buffer.data(),
                requested
            );

        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }

        if (received <= 0) {
            ingress_.cancel();
            sendJson(
                request,
                "400 Bad Request",
                "{\"error\":\"upload_interrupted\"}"
            );
            return ESP_OK;
        }

        result =
            ingress_.appendChunk(
                std::string_view(
                    buffer.data(),
                    static_cast<std::size_t>(
                        received
                    )
                )
            );

        if (!result.ok()) {
            ingress_.cancel();
            sendJson(
                request,
                "500 Internal Server Error",
                std::string("{\"error\":\"") +
                    uploadStatusName(result.status) +
                    "\"}"
            );
            return ESP_OK;
        }

        remaining -=
            static_cast<std::size_t>(
                received
            );
    }

    result =
        ingress_.finish(
            nextAddedOrder()
        );

    if (!result.ok()) {
        const char* response_status =
            result.import_status ==
                StagedImportStatus::Duplicate
                ? "409 Conflict"
                : result.import_status ==
                    StagedImportStatus::UnsupportedFormat
                    ? "415 Unsupported Media Type"
                    : "422 Unprocessable Content";

        sendJson(
            request,
            response_status,
            std::string("{\"error\":\"") +
                uploadStatusName(result.status) +
                "\",\"import_status\":" +
                std::to_string(
                    static_cast<unsigned>(
                        result.import_status
                    )
                ) +
                "}"
        );
        return ESP_OK;
    }

    upload_completed_.store(true);

    sendJson(
        request,
        "201 Created",
        std::string("{\"status\":\"ok\",\"book_id\":\"") +
            result.book_id +
            "\"}"
    );

    ESP_LOGI(
        kTag,
        "Book uploaded: %s",
        result.book_id.c_str()
    );

    return ESP_OK;
}

} // namespace enku::platform::esp_idf
