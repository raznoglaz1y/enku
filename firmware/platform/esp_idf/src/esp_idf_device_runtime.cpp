#include "enku/platform/esp_idf/esp_idf_device_runtime.hpp"

namespace enku::platform::esp_idf {

EspIdfDeviceRuntime::EspIdfDeviceRuntime(
    EspIdfPlatform& platform,
    TypographySettings typography,
    Viewport viewport
)
    : platform_(platform),
      storage_(
          platform_.stateFiles(),
          platform_.bookFiles()
      ),
      text_renderer_(
          platform_.framebuffer()
      ),
      reader_(
          storage_,
          platform_.refresh(),
          text_renderer_,
          text_renderer_,
          text_renderer_,
          typography,
          viewport
      ),
      sleep_wake_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          platform_.network(),
          platform_.power(),
          reader_.bootRestore()
      ),
      power_off_(
          storage_.appState(),
          storage_.library(),
          storage_.checkpoints(),
          storage_.appContext(),
          platform_.network(),
          platform_.power()
      ),
      input_dispatcher_(
          storage_.appState(),
          reader_.library(),
          reader_.reader(),
          sleep_wake_,
          power_off_
      ) {}

DeviceRuntimeInitStatus
EspIdfDeviceRuntime::begin() {
    const auto font_status =
        text_renderer_.begin(
            storage_.appState().
                typography.font_size_px
        );

    if (font_status ==
        FontInitStatus::FontNotFound) {
        return DeviceRuntimeInitStatus::FontMissing;
    }

    if (font_status != FontInitStatus::Ok) {
        return DeviceRuntimeInitStatus::FontInitFailed;
    }

    boot_result_ = reader_.bootRestore().run();

    if (boot_result_.status ==
        BootRestoreStatus::RecoveryRequired) {
        return DeviceRuntimeInitStatus::RecoveryRequired;
    }

    return DeviceRuntimeInitStatus::Ok;
}

ApplicationStorageRuntime&
EspIdfDeviceRuntime::storage() {
    return storage_;
}

ApplicationReaderRuntime&
EspIdfDeviceRuntime::reader() {
    return reader_;
}

FreeTypeTextRenderer&
EspIdfDeviceRuntime::textRenderer() {
    return text_renderer_;
}

SleepWakeCoordinator&
EspIdfDeviceRuntime::sleepWake() {
    return sleep_wake_;
}

PowerOffCoordinator&
EspIdfDeviceRuntime::powerOff() {
    return power_off_;
}

InputDispatcher&
EspIdfDeviceRuntime::input() {
    return input_dispatcher_;
}

InputDispatchResult EspIdfDeviceRuntime::pollInput(
    std::uint32_t now_ms
) {
    const auto event =
        platform_.buttons().poll(now_ms);

    if (!event.has_value()) {
        return InputDispatchResult::Ignored;
    }

    return input_dispatcher_.handle(*event);
}

const BootRestoreResult&
EspIdfDeviceRuntime::bootResult() const {
    return boot_result_;
}

} // namespace enku::platform::esp_idf
