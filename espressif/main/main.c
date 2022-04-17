// This file is part of switchbota (https://github.com/kendallgoto/switchbota/).
// Copyright (c) 2022 Kendall Goto.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "wifi.h"
#include "ota.h"

static const char *TAG = "switchbota";

void start_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ESP_LOGI(TAG, "NVS ready ...");
}

void app_main(void)
{
    start_nvs();

    ESP_LOGI(TAG, "Starting wifi task ...");
    wifi_init();

    while(!wifi_is_connected()) { vTaskDelay(1000 / portTICK_PERIOD_MS); }

    ESP_LOGI(TAG, "Connected to network. Starting OTA task ...");
    esp_ota_mark_app_valid_cancel_rollback(); // only confirm OTA if we can still get online
    ota_init();
    while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
}
