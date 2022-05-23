# SwitchbOTA
Replaces the default firmware on the SwitchBot Plug Mini via OTA, enabling the use of Tasmota without disassembling the unit.

Similar to [Espressif2Arduino](https://github.com/khcnz/Espressif2Arduino).

## Disclaimer
Writing to the bootloader over OTA is dangerous and not normally done for good reason. If your device loses power or somehow flashes corrupted data, the device will be bricked and require disassembly to reprogram the device. Only perform this process if you are comfortable disassembling your plug to fix it if it breaks! Of course, do not unplug or otherwise disturb the plug while it is performing the OTA.

This is a proof of concept and provided with no warranty.

## Pre-reqs
In order to have the factory firmware download our custom firmware, you'll need to modify your local DNS to serve the desired binary. Specifically, `www.wohand.com` should point to a local machine running the included [web server](/server). If you don't know what this means, read [here](https://github.com/kendallgoto/switchbota/issues/3#issuecomment-1121828064) for more information.

## Install
1. Setup the [web server](/server) to serve the desired OTA binaries to your device.
2. Build the [ESP-IDF binary](/espressif) or download the Release binary to flash the device.
3. Trigger an update via the product's app.
4. Connect to the Tasmota hotspot and configure!

## Tasmota Setup
Currently, Tasmota's support for the ESP32-C3 is unofficial. However, power monitoring and MQTT delivery works effectively with the following template:

```json
{"NAME":"W1901400","GPIO":[0,0,32,0,0,0,224,320,321,0,0,0,0,0,0,0,0,0,2720,2656,2624,0],"FLAG":0,"BASE":1}
```
[More Template information](https://templates.blakadder.com/switchbot_plugmini_W1901400.html)

[How to use a template?](https://templates.blakadder.com/howto.html)

## Process
The included ESP-IDF code is a lightweight OTA client that directly writes to the embedded flash chip, enabling the install of non-app level code, including modfiying the bootloader and partiton table.

1. The update is triggered by the app with a BLE message.
2. The device fetches `http://www.wohand.com/version/wocaotech/firmware/WoPlugUS/WoPlugUS_VXX.bin` for the firmware.
3. The request is intercepted and served by our web server, which downloads the espressif binary
4. The factory firmware installs the binary to ota_0 or ota_1, depending on past usage.
5. The binary runs from its OTA partition. If it's on ota_1, it skips to step 6. Otherwise, it performs another OTA, downloading itself into ota_1 and rebooting.
6. Once on OTA_1, the device fetches `http://www.wohand.com/payload.bin` for OTA.
7. The request is intercepted and served by our web server, delivering our desired payload.
8. The espressif binary wipes the internal flash and flashes the payload. It performs two checksums, one of the downloaded binary and another of the flashed binary. It will continue to retry without restarting until the checksums are valid.

## Troubleshooting
WiFi configuration is read from the device's NVS memory by the Espressif binary. This should be configured in the SwitchBot app prior to the process. If, for whatever reason, it is lost, the binary will make a fallback connection to SSID `switchbota`, password `switchbota`. You may need to create this SSID to recover the device.

If you're already on the latest version of the official firmware, you won't be able to begin the OTA process from the app. If this is the case, you can manually start the process by sending bluetooth signals directly to the plug. Read [here](https://github.com/kendallgoto/switchbota/issues/3#issuecomment-1121864522) for more information.
