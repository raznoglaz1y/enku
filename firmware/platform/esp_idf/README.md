# ENKU ESP-IDF Platform Bring-up

This directory is the first board-specific ENKU target for the Waveshare ESP32-S3-ePaper-3.97.

It intentionally starts with storage only. Display, input, PMU and Wi-Fi adapters will be added after this baseline builds and runs on the real board.

## Verified board assumptions

Current board constants come from the official Waveshare ESP32-S3-ePaper-3.97 documentation/examples.

### TF / SDMMC

```text
CLK  GPIO43
CMD  GPIO44
D0   GPIO39
D1   GPIO40
D2   GPIO41
D3   GPIO42
```

ENKU mounts the FAT filesystem at:

```text
/sdcard
```

and ensures:

```text
/sdcard/books
/sdcard/system
/sdcard/system/state
/sdcard/system/covers
/sdcard/system/tmp
```

### E-paper pins reserved for the next bring-up step

```text
BUSY GPIO3
DC   GPIO9
CS   GPIO10
SCLK GPIO11
MOSI GPIO12
RST  GPIO46
```

The current target does not drive the panel yet.

## Build

Install and activate a current ESP-IDF environment, then:

```bash
cd firmware/platform/esp_idf
idf.py set-target esp32s3
idf.py build
```

## Flash and monitor

```bash
idf.py -p <PORT> flash monitor
```

Exit the monitor with the normal ESP-IDF monitor shortcut.

## Expected first boot

With a FAT32 TF card inserted, serial output should show:

1. ENKU ESP-IDF platform bring-up;
2. free heap at boot;
3. SD/MMC card information;
4. successful mount at `/sdcard`;
5. `Storage smoke test passed`;
6. `ENKU platform storage bring-up complete`.

The smoke test creates:

```text
/system/tmp/platform-smoke.txt
```

reads it back, verifies it appears in directory enumeration, and deletes it.

## Platform storage adapter

`EspIdfFileStore` implements both framework-neutral interfaces:

- `StateFileStore`;
- `BookFileStore`.

Logical ENKU paths such as:

```text
/system/settings.a.cbor
/books/book-123.txt
```

are resolved beneath `/sdcard`.

The adapter rejects path traversal containing `..`.

This means the existing CBOR Library/settings/checkpoint/import code can later be connected without changing its filesystem-facing APIs.

## Current limitations

This first target deliberately does not yet:

- instantiate the complete ENKU runtime graph;
- render the e-paper display;
- scan buttons;
- initialize PMU/battery state;
- configure Wi-Fi;
- verify fsync/power-loss durability on the physical TF card.

Those are separate bring-up steps.
