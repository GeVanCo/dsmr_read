#ifndef DSMR_PARSER_H
#define DSMR_PARSER_H

#include <stdbool.h>
#include <stddef.h>

//
// OBIS identifiers
//
typedef enum
{
    OBIS_NONE = 0,
    OBIS_1_8_0,
    OBIS_2_8_0,
    OBIS_32_7_0,
    OBIS_52_7_0,
    OBIS_72_7_0,
    OBIS_31_7_0,
    OBIS_51_7_0,
    OBIS_71_7_0,
    OBIS_1_6_0,
    OBIS_24_2_1_GAS,
    OBIS_24_2_3_WATER,
    OBIS_24_2_4_HEAT,
    OBIS_2_7_0_SOLAR,
    OBIS_1_0_0_TIMESTAMP,
    OBIS_96_14_0_TARIFF,
    OBIS_96_3_10_BREAKER
} obis_id_t;

//
// Optional data struct (keep if you still want it)
//
typedef struct
{
    float power_import;
    float power_export;

    float voltage_l1;
    float voltage_l2;
    float voltage_l3;

    float current_l1;
    float current_l2;
    float current_l3;

    float monthly_peak_kw;

    float gas_m3;
    float water_m3;
    float heat_gj;
    float solar_kwh;

    char  timestamp[16];
}
dsmr_data_t;

//
// OBIS descriptor: enum ↔ prefix ↔ JSON key
//
typedef struct
{
    obis_id_t    id;
    const char  *prefix;    // line prefix to match
    const char  *json_key;  // JSON key used in json_p1_set_obis()
}
obis_descriptor_t;

extern const obis_descriptor_t dsmr_obis_table[];
extern const size_t            dsmr_obis_table_count;

//
// Main parser
//
dsmr_data_t dsmr_parse(char *telegram);

#endif // DSMR_PARSER_H
