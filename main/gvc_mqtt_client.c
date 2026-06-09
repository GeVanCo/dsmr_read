#include "gvc_mqtt_client.h"

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_event.h"
#include "system_status.h"

#include <stdio.h>

static const char *TAG = "gvc_mqtt_client";
static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;

#define MQTT_URI      "mqtt://192.168.1.43"   // change to your broker
#define MQTT_TOPIC    "home/dsmr/data"

// ---------------------------------------------------------------------------
// Payload buffer size for MQTT JSON messages.
// 1024 bytes gives plenty of headroom for future expansion (extra OBIS values,
// multi-phase data, timestamps, metadata, etc.) without risking truncation.
// ---------------------------------------------------------------------------
#define MQTT_PAYLOAD_BUFFER_SIZE 1024

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

void mqtt_publish_dsmr(const dsmr_data_t *data)
{
    // Layer 1: MQTT client exists
    if (!client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return;
    }

    // Layer 2: MQTT is connected
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, skipping publish");
        return;
    }

    char payload[MQTT_PAYLOAD_BUFFER_SIZE];
    int offset = 0;

    offset += snprintf(payload + offset, sizeof(payload) - offset,
                    "{"
                    "\"import\":%.3f,"
                    "\"export\":%.3f,",
                    data->power_import,
                    data->power_export);

    // ------------------------------------------------------------
    // Electricity: 1‑phase or 3‑phase
    // ------------------------------------------------------------
    if (data->features.isThreePhase) {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"voltage_l1\":%.1f,"
                        "\"voltage_l2\":%.1f,"
                        "\"voltage_l3\":%.1f,"
                        "\"current_l1\":%.1f,"
                        "\"current_l2\":%.1f,"
                        "\"current_l3\":%.1f,",
                        data->voltage_l1,
                        data->voltage_l2,
                        data->voltage_l3,
                        data->current_l1,
                        data->current_l2,
                        data->current_l3);
    } else {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"voltage_l1\":%.1f,"
                        "\"current_l1\":%.1f,",
                        data->voltage_l1,
                        data->current_l1);
    }

    // ------------------------------------------------------------
    // Gas
    // ------------------------------------------------------------
    if (data->features.hasGas) {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"gas_m3\":%.3f,",
                        data->gas_m3);
    }

    // ------------------------------------------------------------
    // Water
    // ------------------------------------------------------------
    if (data->features.hasWater) {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"water_m3\":%.3f,",
                        data->water_m3);
    }

    // ------------------------------------------------------------
    // Heat
    // ------------------------------------------------------------
    if (data->features.hasHeat) {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"heat_gj\":%.3f,",
                        data->heat_gj);
    }

    // ------------------------------------------------------------
    // Solar export
    // ------------------------------------------------------------
    if (data->features.hasSolar) {
        offset += snprintf(payload + offset, sizeof(payload) - offset,
                        "\"solar_export\":%.3f,",
                        data->power_export);
    }

    // ------------------------------------------------------------
    // Remove trailing comma and close JSON
    // ------------------------------------------------------------
    if (payload[offset - 1] == ',') {
        offset--;  // remove last comma
    }

    snprintf(payload + offset, sizeof(payload) - offset, "}");

    // ----------------------------------------------------------------------
    // Detect truncation (snprintf returns the number of chars that *would*
    // have been written). If too large, publish an error JSON instead.
    // ----------------------------------------------------------------------
    if (offset >= sizeof(payload)) {
        ESP_LOGE(TAG,
                 "MQTT payload truncated! ([%d] bytes needed, max allowed: [%d])",
                 offset, 
                 MQTT_PAYLOAD_BUFFER_SIZE
                );

        // Build a small JSON error message
        snprintf(
            payload, 
            sizeof(payload),
            "{"
            "\"error\":\"payload_too_large\","
            "\"required\":%d,"
            "\"max_allowed\":%d"
            "}",
            offset, 
            MQTT_PAYLOAD_BUFFER_SIZE
        );
    }
    
    int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, payload, 0, 1, 0);

    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published DSMR data: %s", payload);
    } else {
        ESP_LOGE(TAG, "Failed to publish DSMR data");
    }
}

