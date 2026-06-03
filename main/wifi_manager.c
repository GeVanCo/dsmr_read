#include "wifi_manager.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "led_status.h"

static const char *TAG = "wifi_manager";

#define WIFI_SSID      "SisyPhus"
#define WIFI_PASS      "Vision#Tpv"
#define WIFI_MAX_RETRY 5

static int s_retry_num = 0;
static bool s_connected = false;

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGE(TAG, "connect to the AP fail");
            led_status_set(LED_STATUS_WIFI_DISCONNECTED);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));
        s_connected = true;
        s_retry_num = 0;
        led_status_set(LED_STATUS_HEARTBEAT);
    }
}

static void set_wifi_power(int8_t power_value)
{
    esp_err_t err = esp_wifi_set_max_tx_power(power_value);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to set TX power, err = %d", err);
        ESP_LOGE(TAG, "Error name = [%s]", esp_err_to_name(err));
    }
    else
    {
        ESP_LOGI(TAG, "WiFi TX power set to [%.2f] dBm", power_value * 0.25f);
    }
}

void wifi_manager_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                               ESP_EVENT_ANY_ID,
                                               &wifi_event_handler,
                                               NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                               IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler,
                                               NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Reduce the transmit power of the board to avoid bad behaviour
    // NOTE: SETTING THE TRANSMIT POWER MUST BE DONE ***AFTER*** THE esp_wifi_start() CALL!!!!
    set_wifi_power(34);   // 8.5 dBm: 34 x 0.25 dBm (steps are 0.25dBm)    

    ESP_LOGI(TAG, "wifi init finished, connecting to [%s]", WIFI_SSID);

    // Simple blocking wait until connected or retries exhausted
    int wait_ms = 0;
    while (!s_connected && s_retry_num < WIFI_MAX_RETRY && wait_ms < 30000) {
        vTaskDelay(pdMS_TO_TICKS(100));
        wait_ms += 100;
    }

    if (!s_connected) {
        ESP_LOGE(TAG, "WiFi connection failed");
    } else {
        ESP_LOGI(TAG, "WiFi connected");
    }
}
