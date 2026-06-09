#pragma once

typedef struct {
    bool isThreePhase;
    bool hasGas;
    bool hasWater;
    bool hasHeat;
    bool hasSolar;
} MeterFeatures;

typedef struct {
    float power_import;   // 1-0:1.8.0
    float power_export;   // 1-0:2.8.0

    float voltage_l1;     // 1-0:32.7.0
    float voltage_l2;     // 1-0:52.7.0
    float voltage_l3;     // 1-0:72.7.0

    float current_l1;     // 1-0:31.7.0
    float current_l2;     // 1-0:51.7.0
    float current_l3;     // 1-0:71.7.0

    float gas_m3;         // 0-1:24.2.1
    float water_m3;       // 0-2:24.2.3   
    float heat_gj;        // 0-3:24.2.4
    float solar_kwh;      // 1-0:2.7.0

    MeterFeatures features;
} dsmr_data_t;

dsmr_data_t dsmr_parse(const char *telegram);

