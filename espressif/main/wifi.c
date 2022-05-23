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
#include <string.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi.h"
#include "config.h"

static const char *TAG = "wireless";

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wifi_task(void *pvParameters);

static void wifi_set_connected(uint8_t c) {
    if (c) {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }

}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    switch(event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_STA_START");
            esp_wifi_connect();
            break;
            
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_STA_GOT_IP");
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            wifi_set_connected(1);
            break;
            
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_STA_DISCONNECTED");
            // try to re-connect
            esp_wifi_connect();
            wifi_set_connected(0);
            break;
        
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_AP_START");
            break;
        
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_AP_STADISCONNECTED");
            break;
            
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_AP_STACONNECTED");
            break;
        
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_AP_STADISCONNECTED");
            break;
        
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGI(TAG, "event_handler: SYSTEM_EVENT_AP_STADISCONNECTED");
            break;
            
        default:
            break;        
    }
    return;
}
int wifi_is_connected()
{
    return xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT;
}
void wifi_connect()
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // get default config ...
    wifi_config_t existing_config = { 0 };
    esp_wifi_get_config(WIFI_IF_STA, &existing_config);
    ESP_LOGI(TAG, "%s", existing_config.sta.ssid);
    if(strlen((char *)existing_config.sta.ssid) == 0 || strcmp((char *)existing_config.sta.ssid, "wocao_factory_test") == 0) {
        ESP_LOGI(TAG, "defaulting to fallback network ...");
        wifi_config_t fallback_config = {
            .sta = {
                .ssid = FALLBACK_SSID,
                .password = FALLBACK_PASS
            }
        };
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &fallback_config));
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());
}

void wifi_init()
{
    ESP_LOGI(TAG, "wifi_init");
    
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    tcpip_adapter_init();

    s_wifi_event_group = xEventGroupCreate();
    xTaskCreate(&wifi_task, "wifiTask", 32768, NULL, 5, NULL);
    wifi_connect();
}

void wifi_task(void *pvParameters)
{
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "networkTask: connected to access point");

    while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
}

