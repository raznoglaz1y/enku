#pragma once

#include <cstdint>

namespace enku::platform::esp_idf {

enum class InternalStateMountStatus : std::uint8_t {
    Ok,
    MountFailed,
};

class EspIdfInternalStateStorage {
public:
    EspIdfInternalStateStorage() = default;
    ~EspIdfInternalStateStorage();

    InternalStateMountStatus begin();

    bool mounted() const;

    static constexpr const char* kMountPoint =
        "/state";
    static constexpr const char* kPartitionLabel =
        "state";

private:
    bool mounted_{false};
};

} // namespace enku::platform::esp_idf
