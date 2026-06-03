#pragma once

typedef struct {
    float power_import;   // 1-0:1.8.0
    float power_export;   // 1-0:2.8.0
    float voltage_l1;     // 1-0:32.7.0
    float current_l1;     // 1-0:31.7.0
} dsmr_data_t;

dsmr_data_t dsmr_parse(const char *telegram);

