#include "gvc_mqtt_client.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "led_status.h"

#include <stdio.h>

static const char *TAG = "mqtt_client";
static esp_mqtt_client_handle_t client = NULL;

#define MQTT_URI      "mqtt://192.168.1.43"   // change to your broker
#define MQTT_TOPIC    "home/dsmr/data"

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            led_status_set(LED_STATUS_HEARTBEAT);
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            led_status_set(LED_STATUS_MQTT_DISCONNECTED);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

void mqtt_client_init(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_URI,
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

void mqtt_publish_dsmr(const dsmr_data_t *data)
{
    if (!client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{"
             "\"import\":%.3f,"
             "\"export\":%.3f,"
             "\"voltage\":%.1f,"
             "\"current\":%.1f"
             "}",
             data->power_import,
             data->power_export,
             data->voltage_l1,
             data->current_l1);

    int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, payload, 0, 1, 0);

    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published DSMR data: %s", payload);
    } else {
        ESP_LOGE(TAG, "Failed to publish DSMR data");
    }
}

