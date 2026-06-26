#include "gvc_mqtt_client.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "system_status.h"

#include <stdio.h>

static const char *TAG = "#_client";
static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;

#define MQTT_URI      "mqtt://192.168.1.43"   // change to your broker
#define MQTT_TOPIC    "home/dsmr/data"

// ---------------------------------------------------------------------------
// Payload buffer size for MQTT JSON messages.
// 1024 bytes gives plenty of headroom for future expansion (extra OBIS values,
// multi-phase data, timestamps, metadata, etc.) without risking truncation.
// ---------------------------------------------------------------------------
#define MQTT_PAYLOAD_BUFFER_SIZE 2048

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT connected");
            system_status_set(SYSTEM_STATUS_MQTT, true);
            break;

        case MQTT_EVENT_DISCONNECTED:
            mqtt_connected = false;
            ESP_LOGW(TAG, "MQTT disconnected");
            system_status_set(SYSTEM_STATUS_MQTT, false);
            break;

        case MQTT_EVENT_ERROR:
            mqtt_connected = false;
            ESP_LOGE(TAG, "MQTT error");
            system_status_set(SYSTEM_STATUS_MQTT, false);
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            ESP_LOGW(TAG, "MQTT trying to connect...");
            break;

        default:
            break;
    }
}

void mqtt_client_init(void) 
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
        .network.disable_auto_reconnect = false,   // ensure auto-reconnect is enabled
    };

    client = esp_mqtt_client_init(&cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL));

    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    ESP_LOGI(TAG, "MQTT client started");
}

void mqtt_publish_dsmr(const char *json)
{
    if (!client)
    {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }

    if (!mqtt_connected)
    {
        ESP_LOGW(TAG, "MQTT not connected, skipping publish");
        return;
    }

    if (!json)
    {
        ESP_LOGE(TAG, "JSON payload is NULL");
        return;
    }

    int msg_id = esp_mqtt_client_publish(
        client,
        MQTT_TOPIC,
        json,
        0,      // payload length (0 = auto)
        1,      // QoS
        0       // retain
    );

    if (msg_id >= 0)
    {
        ESP_LOGI(TAG, "Published DSMR JSON: %s", json);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to publish DSMR JSON");
    }
}
