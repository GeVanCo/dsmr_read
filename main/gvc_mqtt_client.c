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
    size_t offset = 0;
    bool truncated = false;

    // Helper macro/function to write safely to buffer
    #define SAFE_APPEND(...) do { \
        if (!truncated) { \
            int ret = snprintf(payload + offset, sizeof(payload) - offset, __VA_ARGS__); \
            if (offset + (size_t)ret >= sizeof(payload)) { \
                truncated = true; \
            } else { \
                offset += (size_t)ret; \
            } \
        } \
    } while(0)

    // Start building JSON payload
    SAFE_APPEND("{");

    // 1. Electricity import/export is always present
  SAFE_APPEND("\"I\":%.3f,", data->power_import);
  SAFE_APPEND("\"E\":%.3f,", data->power_export);

    // 2. Electricity: 1-phase or 3-phase
    if (data->features.isThreePhase) {
        SAFE_APPEND("\"V_L1\":%.1f,", data->voltage_l1);
        SAFE_APPEND("\"V_L2\":%.1f,", data->voltage_l2);
        SAFE_APPEND("\"V_L3\":%.1f,", data->voltage_l3);
        SAFE_APPEND("\"I_L1\":%.1f,", data->current_l1);
        SAFE_APPEND("\"I_L2\":%.1f,", data->current_l2);
        SAFE_APPEND("\"I_L3\":%.1f,", data->current_l3);
    } else {
        SAFE_APPEND("\"V_L1\":%.1f,", data->voltage_l1);
        SAFE_APPEND("\"I_L1\":%.1f,", data->current_l1);
    }

    // 3. Optional resources: gas, water, heat, solar, monthly peak
    if (data->features.hasGas) {
    SAFE_APPEND("\"GAS\":%.3f,", data->gas_m3);
    }

    if (data->features.hasWater) {
    SAFE_APPEND("\"H2O\":%.3f,", data->water_m3);
    }

    if (data->features.hasHeat) {
    SAFE_APPEND("\"HEAT\":%.3f,", data->heat_gj);
    }

    if (data->features.hasSolar) {
    SAFE_APPEND("\"Sol\":%.3f,", data->solar_kwh);
    }

    if (data->features.hasMonthlyPeak) {
    SAFE_APPEND("\"M_PEAK\":%.3f,", data->monthly_peak_kw);
    }

    // ------------------------------------------------------------
    // Remove trailing comma and close JSON
    // ------------------------------------------------------------
    if (!truncated) {
        if (offset > 1 && payload[offset - 1] == ',') {
            offset--; // Remove trailing comma
        }
        payload[offset] = '\0'; // Provide correct termination
        snprintf(payload + offset, sizeof(payload) - offset, "}");
    } 
    else {
        // When buffer overloaded (truncated = true) we overwrite with a clean error message
        ESP_LOGE(TAG, "MQTT payload too big!  Buffer overload...");
        
        snprintf(payload, sizeof(payload),
            "{"
            "\"error\":\"payload_too_large\","
            "\"max_allowed\":%d"
            "}",
            MQTT_PAYLOAD_BUFFER_SIZE
        );
    }

    #undef SAFE_APPEND // Clean up the macro

    int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, payload, 0, 1, 0);

    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published DSMR data: %s", payload);
    } else {
        ESP_LOGE(TAG, "Failed to publish DSMR data");
    }
}

