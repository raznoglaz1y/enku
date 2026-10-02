#pragma once

#include <cstdint>

#include "esp_http_server.h"

#include "enku/runtime/web_upload_ingress.hpp"
#include "enku/services/services.hpp"

namespace enku::platform::esp_idf {

class EspIdfWebUploadServer {
public:
    EspIdfWebUploadServer(
        WebUploadIngress& ingress,
        LibraryService& library
    );

    ~EspIdfWebUploadServer();

    bool sync(bool online);
    bool running() const;

private:
    WebUploadIngress& ingress_;
    LibraryService& library_;
    httpd_handle_t server_{nullptr};

    bool start();
    void stop();
    std::uint64_t nextAddedOrder() const;

    static esp_err_t handleIndex(
        httpd_req_t* request
    );

    static esp_err_t handleHealth(
        httpd_req_t* request
    );

    static esp_err_t handleUpload(
        httpd_req_t* request
    );

    esp_err_t upload(
        httpd_req_t* request
    );
};

} // namespace enku::platform::esp_idf
