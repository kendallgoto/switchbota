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

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_spi_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rom/md5_hash.h"

#include "config.h"

#include "ota.h"

static const char *TAG = "ota";

static void ota_task(void *pvParameters);
void ota_init()
{
    ESP_LOGI(TAG, "ota_init");
    
    xTaskCreate(&ota_task, "otaTask", 32768, NULL, 5, NULL);
}

esp_err_t ota_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER");
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}
void dump(const void* data, size_t size)
{
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		printf("%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					printf(" ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					printf("   ");
				}
				printf("|  %s \n", ascii);
			}
		}
	}
}
int verify_checksum(unsigned char *result, unsigned char *comparison)
{
    int equal = 1;
    for(int i = 0; i < 16; i++) {
        if(result[i] != comparison[i]) {
            equal = 0;
        }
    }
    return equal;
}

unsigned char buf[OTA_BUF];

void ota_flip() {
    esp_http_client_config_t config = {
        .url = FALLBACK_URL,
        .event_handler = ota_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_open(client, 0);
    esp_http_client_fetch_headers(client);
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t update_handle = 0;
    int bytesWritten = 0;
    bool first_byte = false;
    while (1) {
        int data_read = esp_http_client_read(client, (char *)buf, OTA_BUF);
        if (data_read > 0) {
            if (first_byte == false) {
                    first_byte = true;
                    esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
            }
            esp_ota_write( update_handle, (const void *)buf, data_read);
            bytesWritten += data_read;
            ESP_LOGD(TAG, "Written image length %d", bytesWritten);
        } else {
            break;
        }
    }
    ESP_LOGI(TAG, "Total Write binary data length: %d", bytesWritten);
    esp_ota_end(update_handle);

    esp_ota_set_boot_partition(update_partition);
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

void ota_inject() {
        ESP_LOGI(TAG, "erasing bootloader! don't lose power now!");
        ESP_ERROR_CHECK( esp_flash_erase_region(NULL, 0, WRITE_SIZE) );

        esp_http_client_config_t config = {
            .url = BINARY_URL,
            .event_handler = ota_event_handler,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_open(client, 0);
        esp_http_client_fetch_headers(client);
        int totalBytes = esp_http_client_get_content_length(client);
        int readBytes = 0;

        struct MD5Context hashCalculate = { 0 };
        unsigned char hashData[16] = { 0 };
        MD5Init(&hashCalculate);
        int count;
        while((count = esp_http_client_read(client, (char *)buf, OTA_BUF)) > 0) {
            MD5Update(&hashCalculate, buf, count); // calculate MD5 as we download
            esp_flash_write(NULL, buf, readBytes, count);
            readBytes += count;
            ESP_LOGI(TAG, "flashed %d / %d bytes", readBytes, totalBytes);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
        MD5Final(hashData, &hashCalculate);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        if(verify_checksum(hashData, BINARY_MD5)) {
            ESP_LOGI(TAG, "Download MD5 OK, verifying SPI");
            // verify written data from SPI
            struct MD5Context spiMD5 = { 0 };
            unsigned char spiHashData[16] = { 0 };
            MD5Init(&spiMD5);
            for(int i = 0; i < totalBytes; i += MIN(OTA_BUF, (totalBytes-i))) {
                int bytesToRead = MIN(OTA_BUF, (totalBytes-i));
                esp_flash_read(NULL, buf, i, bytesToRead);
                MD5Update(&spiMD5, buf, bytesToRead); // calculate MD5 as we download
                //dump(buf, bytesToRead);
                ESP_LOGI(TAG, "read %d / %d bytes", i, totalBytes);
                vTaskDelay(20 / portTICK_PERIOD_MS);
            }
            MD5Final(spiHashData, &spiMD5);
            if(verify_checksum(spiHashData, BINARY_MD5)) {
                ESP_LOGI(TAG, "SPI Write OK - restarting ...");
                esp_restart();
            } else {
                ESP_LOGE(TAG, "SPI Write BAD, retrying ...");
            }
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        } else {
            ESP_LOGE(TAG, "Download BAD, retrying ...");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
}
void ota_task(void *pvParameters)
{
    // check if we're in an ok OTA position ...
    ESP_LOGI(TAG, "ota_task ready");
    const esp_partition_t *currentPartition = esp_ota_get_running_partition();
    if(currentPartition->address < WRITE_SIZE) {
        ESP_LOGI(TAG, "OTA_0 detected!");
        ota_flip();
        return;
    } else {
        while(1) {
            ota_inject();
        }
    }
}

