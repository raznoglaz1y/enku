#include "enku/platform/esp_idf/esp_idf_web_upload_server.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <string>

#include "esp_log.h"
#include "mdns.h"

namespace enku::platform::esp_idf {
namespace {

constexpr const char* kTag = "ENKU_WEB";

constexpr const char* kIndexHtml = R"html(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ENKU Library</title>
<style>
body{font-family:system-ui,sans-serif;max-width:620px;margin:42px auto;padding:0 20px;color:#111}
h1{font-size:28px;margin:0 0 8px}h2{font-size:18px;margin-top:34px}
p{line-height:1.5;color:#555}
input,button{font:inherit}
input{display:block;margin:20px 0 12px;width:100%}
button{padding:10px 14px;border:1px solid #111;background:#111;color:#fff;border-radius:8px}
button:disabled{opacity:.45}
.book{display:flex;gap:14px;align-items:center;padding:14px 0;border-top:1px solid #ddd}
.book main{flex:1;min-width:0}.title{font-weight:650}.meta{font-size:13px;color:#666;margin-top:4px}
.delete{background:#fff;color:#111;padding:7px 10px}
#status{margin-top:16px;white-space:pre-wrap;min-height:22px}
</style>
</head>
<body>
<h1>ENKU</h1>
<p>Upload TXT and EPUB books and manage this reader over the local network.</p>
<input id="file" type="file" accept=".txt,.epub,text/plain,application/epub+zip">
<button id="upload">Upload book</button>
<div id="status"></div>
<h2>Library</h2>
<div id="books">Loading…</div>
<script>
const file=document.getElementById('file');
const button=document.getElementById('upload');
const status=document.getElementById('status');
const books=document.getElementById('books');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function loadLibrary(){
  try{
    const r=await fetch('/api/library',{cache:'no-store'});
    if(!r.ok)throw new Error(await r.text());
    const data=await r.json();
    books.replaceChildren();
    if(!data.books.length){books.textContent='No books yet.';return;}
    for(const b of data.books){
      const row=document.createElement('div');row.className='book';
      const main=document.createElement('main');
      const title=document.createElement('div');title.className='title';
      title.textContent=b.title||b.filename||b.id;
      const meta=document.createElement('div');meta.className='meta';
      const progress=Math.round((b.progress||0)*100);
      meta.textContent=(b.author?b.author+' · ':'')+progress+'% · '+b.state;
      const del=document.createElement('button');del.className='delete';del.textContent='Delete';
      del.onclick=()=>deleteBook(b.id,title.textContent,del);
      main.append(title,meta);row.append(main,del);books.append(row);
    }
  }catch(e){books.textContent='Could not load library.';status.textContent=String(e);}
}
async function deleteBook(id,title,control){
  if(!confirm('Delete “'+title+'” from ENKU?'))return;
  control.disabled=true;status.textContent='Deleting…';
  try{
    const r=await fetch('/api/book/'+encodeURIComponent(id),{method:'DELETE'});
    if(!r.ok)throw new Error(await r.text());
    status.textContent='Delete queued.';
    let finalStatus='pending';
    for(let i=0;i<20;i++){
      await sleep(200);
      const sr=await fetch('/api/delete-status/'+encodeURIComponent(id),{cache:'no-store'});
      if(sr.ok){
        const data=await sr.json();
        finalStatus=data.status;
        if(finalStatus!=='pending')break;
      }
    }
    if(finalStatus!=='ok'){
      throw new Error('delete status: '+finalStatus);
    }
    await loadLibrary();
    status.textContent='Deleted: '+title;
  }catch(e){status.textContent='Delete failed: '+e;control.disabled=false;}
}
button.onclick=async()=>{
  const f=file.files[0];
  if(!f){status.textContent='Choose a TXT file first.';return;}
  button.disabled=true;status.textContent='Uploading…';
  try{
    const r=await fetch('/api/upload',{
      method:'POST',
      headers:{'X-ENKU-Filename':f.name,'Content-Type':'application/octet-stream'},
      body:f
    });
    const t=await r.text();
    if(!r.ok)throw new Error(t);
    status.textContent='Uploaded: '+f.name;
    file.value='';
    await loadLibrary();
  }catch(e){status.textContent='Upload failed: '+e;}
  finally{button.disabled=false;}
};
loadLibrary();
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

std::string jsonEscape(
    std::string_view value
) {
    std::string out;
    out.reserve(value.size() + 8U);

    for (const unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20U) {
                    char escaped[7] = {};
                    std::snprintf(
                        escaped,
                        sizeof(escaped),
                        "\\u%04x",
                        static_cast<unsigned>(ch)
                    );
                    out += escaped;
                } else {
                    out.push_back(
                        static_cast<char>(ch)
                    );
                }
                break;
        }
    }

    return out;
}

const char* readingStateName(
    ReadingState state
) {
    switch (state) {
        case ReadingState::New: return "new";
        case ReadingState::Reading: return "reading";
        case ReadingState::Finished: return "finished";
    }
    return "unknown";
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

bool EspIdfWebUploadServer::takeDeleteRequest(
    std::string& book_id
) {
    std::lock_guard<std::mutex> lock(
        delete_mutex_
    );

    if (pending_delete_book_id_.empty()) {
        return false;
    }

    book_id = std::move(
        pending_delete_book_id_
    );
    pending_delete_book_id_.clear();

    delete_result_book_id_ = book_id;
    delete_result_status_ = "pending";
    return true;
}

void EspIdfWebUploadServer::completeDelete(
    const std::string& book_id,
    const std::string& status
) {
    std::lock_guard<std::mutex> lock(
        delete_mutex_
    );

    delete_result_book_id_ = book_id;
    delete_result_status_ = status;
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

    config.max_uri_handlers = 7;
    config.uri_match_fn =
        httpd_uri_match_wildcard;
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

    httpd_uri_t library_uri = {};
    library_uri.uri = "/api/library";
    library_uri.method = HTTP_GET;
    library_uri.handler =
        &EspIdfWebUploadServer::handleLibrary;
    library_uri.user_ctx = this;

    httpd_uri_t delete_status_uri = {};
    delete_status_uri.uri = "/api/delete-status/*";
    delete_status_uri.method = HTTP_GET;
    delete_status_uri.handler =
        &EspIdfWebUploadServer::handleDeleteStatus;
    delete_status_uri.user_ctx = this;

    httpd_uri_t delete_uri = {};
    delete_uri.uri = "/api/book/*";
    delete_uri.method = HTTP_DELETE;
    delete_uri.handler =
        &EspIdfWebUploadServer::handleDeleteBook;
    delete_uri.user_ctx = this;

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
        ) == ESP_OK &&
        httpd_register_uri_handler(
            server_,
            &library_uri
        ) == ESP_OK &&
        httpd_register_uri_handler(
            server_,
            &delete_uri
        ) == ESP_OK &&
        httpd_register_uri_handler(
            server_,
            &delete_status_uri
        ) == ESP_OK;

    if (!registered) {
        ESP_LOGE(
            kTag,
            "Failed to register HTTP handlers"
        );
        stop();
        return false;
    }

    if (!startMdns()) {
        ESP_LOGW(
            kTag,
            "mDNS unavailable; IP access remains active"
        );
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
    stopMdns();
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

esp_err_t EspIdfWebUploadServer::handleLibrary(
    httpd_req_t* request
) {
    auto* self =
        static_cast<EspIdfWebUploadServer*>(
            request->user_ctx
        );

    return self == nullptr
        ? ESP_FAIL
        : self->library(request);
}

esp_err_t EspIdfWebUploadServer::handleDeleteBook(
    httpd_req_t* request
) {
    auto* self =
        static_cast<EspIdfWebUploadServer*>(
            request->user_ctx
        );

    return self == nullptr
        ? ESP_FAIL
        : self->deleteBook(request);
}

esp_err_t EspIdfWebUploadServer::handleDeleteStatus(
    httpd_req_t* request
) {
    auto* self =
        static_cast<EspIdfWebUploadServer*>(
            request->user_ctx
        );

    return self == nullptr
        ? ESP_FAIL
        : self->deleteStatus(request);
}

esp_err_t EspIdfWebUploadServer::library(
    httpd_req_t* request
) {
    httpd_resp_set_type(
        request,
        "application/json"
    );

    if (httpd_resp_send_chunk(
            request,
            "{\"books\":[",
            HTTPD_RESP_USE_STRLEN
        ) != ESP_OK) {
        return ESP_FAIL;
    }

    LibraryQuery query;
    query.mode = LibraryQueryMode::Browse;
    query.filter = LibraryFilter::All;
    query.sort = LibrarySort::RecentlyAdded;
    query.direction = SortDirection::Descending;
    query.limit = 24;

    bool first = true;
    std::uint32_t offset = 0;

    while (true) {
        query.offset = offset;
        LibraryPage page;

        if (library_.query(query, page) !=
            LibraryStatus::Ok) {
            httpd_resp_send_chunk(
                request,
                nullptr,
                0
            );
            return ESP_FAIL;
        }

        for (const auto& book : page.items) {
            const std::string item =
                std::string(first ? "" : ",") +
                "{\"id\":\"" +
                jsonEscape(book.book_id) +
                "\",\"title\":\"" +
                jsonEscape(book.metadata.title) +
                "\",\"author\":\"" +
                jsonEscape(
                    book.metadata.author_display
                ) +
                "\",\"filename\":\"" +
                jsonEscape(
                    book.source_filename
                ) +
                "\",\"progress\":" +
                std::to_string(book.progress) +
                ",\"state\":\"" +
                readingStateName(
                    book.reading_state
                ) +
                "\"}";

            if (httpd_resp_send_chunk(
                    request,
                    item.c_str(),
                    item.size()
                ) != ESP_OK) {
                return ESP_FAIL;
            }

            first = false;
        }

        offset +=
            static_cast<std::uint32_t>(
                page.items.size()
            );

        if (page.items.empty() ||
            offset >= page.total_matches) {
            break;
        }
    }

    if (httpd_resp_send_chunk(
            request,
            "]}",
            2
        ) != ESP_OK) {
        return ESP_FAIL;
    }

    return httpd_resp_send_chunk(
        request,
        nullptr,
        0
    );
}

esp_err_t EspIdfWebUploadServer::deleteBook(
    httpd_req_t* request
) {
    constexpr std::string_view kPrefix =
        "/api/book/";

    const std::string_view uri(
        request->uri
    );

    if (uri.size() <= kPrefix.size() ||
        uri.substr(0, kPrefix.size()) != kPrefix) {
        sendJson(
            request,
            "400 Bad Request",
            "{\"error\":\"invalid_book_id\"}"
        );
        return ESP_OK;
    }

    const std::string book_id(
        uri.substr(kPrefix.size())
    );

    if (!library_.get(book_id).has_value()) {
        sendJson(
            request,
            "404 Not Found",
            "{\"error\":\"not_found\"}"
        );
        return ESP_OK;
    }

    {
        std::lock_guard<std::mutex> lock(
            delete_mutex_
        );

        if (!pending_delete_book_id_.empty()) {
            sendJson(
                request,
                "409 Conflict",
                "{\"error\":\"delete_busy\"}"
            );
            return ESP_OK;
        }

        pending_delete_book_id_ = book_id;
        delete_result_book_id_.clear();
        delete_result_status_.clear();
    }

    sendJson(
        request,
        "202 Accepted",
        "{\"status\":\"queued\"}"
    );
    return ESP_OK;
}

esp_err_t EspIdfWebUploadServer::deleteStatus(
    httpd_req_t* request
) {
    constexpr std::string_view kPrefix =
        "/api/delete-status/";

    const std::string_view uri(
        request->uri
    );

    if (uri.size() <= kPrefix.size() ||
        uri.substr(0, kPrefix.size()) != kPrefix) {
        sendJson(
            request,
            "400 Bad Request",
            "{\"error\":\"invalid_book_id\"}"
        );
        return ESP_OK;
    }

    const std::string book_id(
        uri.substr(kPrefix.size())
    );

    std::lock_guard<std::mutex> lock(
        delete_mutex_
    );

    if (pending_delete_book_id_ == book_id) {
        sendJson(
            request,
            "200 OK",
            "{\"status\":\"pending\"}"
        );
        return ESP_OK;
    }

    if (delete_result_book_id_ == book_id &&
        !delete_result_status_.empty()) {
        sendJson(
            request,
            "200 OK",
            std::string("{\"status\":\"") +
                jsonEscape(delete_result_status_) +
                "\"}"
        );
        return ESP_OK;
    }

    sendJson(
        request,
        "404 Not Found",
        "{\"status\":\"unknown\"}"
    );
    return ESP_OK;
}

bool EspIdfWebUploadServer::startMdns() {
    if (mdns_started_) {
        return true;
    }

    if (mdns_init() != ESP_OK) {
        return false;
    }

    if (mdns_hostname_set("enku") != ESP_OK ||
        mdns_instance_name_set("ENKU Reader") != ESP_OK ||
        mdns_service_add(
            "ENKU Web Library",
            "_http",
            "_tcp",
            80,
            nullptr,
            0
        ) != ESP_OK) {
        mdns_free();
        return false;
    }

    mdns_started_ = true;

    ESP_LOGI(
        kTag,
        "mDNS active at http://enku.local"
    );

    return true;
}

void EspIdfWebUploadServer::stopMdns() {
    if (!mdns_started_) {
        return;
    }

    mdns_free();
    mdns_started_ = false;
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
