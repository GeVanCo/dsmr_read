#pragma once

#include <stdbool.h>

// Subsystem identifiers for unified status updates
typedef enum {
    SYSTEM_STATUS_WIFI = 0,
    SYSTEM_STATUS_MQTT = 1,
    SYSTEM_STATUS_DSMR = 2
} system_status_item_t;

// High-level health of subsystems
typedef struct {
    bool wifi_ok;
    bool mqtt_ok;
    bool dsmr_ok;

    // Tracks which subsystem most recently reported NOT OK
    system_status_item_t last_bad_item;
} system_status_t;

// Initialize system status (all false at boot)
void system_status_init(void);

// Unified setter: update one subsystem's status
void system_status_set(system_status_item_t item, bool ok);

// Optional: get a copy of the current system status
system_status_t system_status_get(void);
