#include "dsmr_parser.h"
#include "json_p1.h"

#include "esp_log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "#_parser";

//
// OBIS descriptor table: enum → prefix → JSON key
//
const obis_descriptor_t dsmr_obis_table[] =
{
    { OBIS_1_8_0,        "1-0:1.8.0",     "1.8.1" },
    { OBIS_2_8_0,        "1-0:2.8.0",     "1.8.2" },

    { OBIS_32_7_0,       "1-0:32.7.0",    "32.7.0" },
    { OBIS_52_7_0,       "1-0:52.7.0",    "52.7.0" },
    { OBIS_72_7_0,       "1-0:72.7.0",    "72.7.0" },

    { OBIS_31_7_0,       "1-0:31.7.0",    "31.7.0" },
    { OBIS_51_7_0,       "1-0:51.7.0",    "51.7.0" },
    { OBIS_71_7_0,       "1-0:71.7.0",    "71.7.0" },

    { OBIS_1_6_0,        "1-0:1.6.0",     "1.6.0" },

    { OBIS_24_2_1_GAS,   "0-1:24.2.1",    "24.2.1_gas" },
    { OBIS_24_2_3_WATER, "0-2:24.2.3",    "24.2.1_water" },
    { OBIS_24_2_4_HEAT,  "0-3:24.2.4",    "24.2.1_heat" },

    { OBIS_2_7_0_SOLAR,  "1-0:2.7.0",     "2.7.0" },

    { OBIS_1_0_0_TIMESTAMP, "0-0:1.0.0",  "1.0.0" },
    { OBIS_96_14_0_TARIFF,  "0-0:96.14.0","96.14.0" },
    { OBIS_96_3_10_BREAKER, "0-0:96.3.10","96.3.10" },
};

const size_t dsmr_obis_table_count =
    sizeof(dsmr_obis_table) / sizeof(dsmr_obis_table[0]);

    static const obis_descriptor_t *get_obis_desc(obis_id_t id)
{
    for (size_t i = 0; i < dsmr_obis_table_count; i++)
    {
        if (dsmr_obis_table[i].id == id)
        {
            return &dsmr_obis_table[i];
        }
    }
    return NULL;
}

//
// Identify OBIS code by prefix match
//
static obis_id_t identify_obis(const char *line)
{
    for (size_t i = 0; i < dsmr_obis_table_count; i++)
    {
        size_t len = strlen(dsmr_obis_table[i].prefix);

        if (strncmp(line, dsmr_obis_table[i].prefix, len) == 0)
        {
            return dsmr_obis_table[i].id;
        }
    }

    return OBIS_NONE;
}

//
// Extract float between '(' and '*' (standard DSMR format)
//
static float extract_float(const char *line)
{
    const char *start = strchr(line, '(');
    const char *end   = strchr(line, '*');

    if (!start || !end || end <= start)
    {
        return 0.0f;
    }

    char buf[32];
    size_t len = end - (start + 1);

    if (len >= sizeof(buf))
    {
        len = sizeof(buf) - 1;
    }

    memcpy(buf, start + 1, len);
    buf[len] = '\0';

    return atof(buf);
}

//
// Extract second (...) value (gas/water/heat/solar/peak)
//
static float extract_second_value(const char *line)
{
    const char *first_close = strchr(line, ')');
    if (!first_close)
    {
        return 0.0f;
    }

    const char *start = strchr(first_close + 1, '(');
    if (!start)
    {
        return 0.0f;
    }

    const char *end = strchr(start, '*');
    if (!end || end <= start + 1)
    {
        return 0.0f;
    }

    char buf[32];
    size_t len = end - (start + 1);

    if (len >= sizeof(buf))
    {
        len = sizeof(buf) - 1;
    }

    memcpy(buf, start + 1, len);
    buf[len] = '\0';

    return atof(buf);
}

//
// Extract timestamp from 0-0:1.0.0
//
static void extract_timestamp(const char *line)
{
    const char *start = strchr(line, '(');
    if (!start)
    {
        return;
    }

    char ts[16];
    strncpy(ts, start + 1, 13);
    ts[13] = '\0';

    const obis_descriptor_t *desc = get_obis_desc(OBIS_1_0_0_TIMESTAMP);
    if (desc)
    {
        json_p1_set_obis(desc->json_key, ts);
    }
}

//
// Main DSMR parser
//
dsmr_data_t dsmr_parse(char *telegram)
{
    dsmr_data_t data = {0};

    if (!telegram)
    {
        ESP_LOGE(TAG, "Telegram pointer is NULL");
        return data;
    }

    char *line = strtok(telegram, "\r\n");

    while (line)
    {
        obis_id_t id = identify_obis(line);

        switch (id)
        {
            case OBIS_1_8_0:
            case OBIS_2_8_0:
            {
                float v = extract_float(line);

                char buf[16];
                snprintf(buf, sizeof(buf), "%.3f", v);

                const obis_descriptor_t *desc = get_obis_desc(id);
                if (desc)
                {
                    json_p1_set_obis(desc->json_key, buf);
                }               
                break;
            }

            case OBIS_32_7_0:
            case OBIS_52_7_0:
            case OBIS_72_7_0:
            case OBIS_31_7_0:
            case OBIS_51_7_0:
            case OBIS_71_7_0:
            {
                float v = extract_float(line);

                char buf[16];
                snprintf(buf, sizeof(buf), "%.1f", v);

                const obis_descriptor_t *desc = get_obis_desc(id);
                if (desc)
                {
                    json_p1_set_obis(desc->json_key, buf);
                }               
                break;
            }

            case OBIS_1_6_0:
            {
                float v = extract_second_value(line);

                char buf[16];
                snprintf(buf, sizeof(buf), "%.3f", v);

                const obis_descriptor_t *desc = get_obis_desc(id);
                if (desc)
                {
                    json_p1_set_obis(desc->json_key, buf);
                }               
                break;
            }

            case OBIS_24_2_1_GAS:
            case OBIS_24_2_3_WATER:
            case OBIS_24_2_4_HEAT:
            case OBIS_2_7_0_SOLAR:
            {
                float v = extract_second_value(line);

                char buf[16];
                snprintf(buf, sizeof(buf), "%.3f", v);

                const obis_descriptor_t *desc = get_obis_desc(id);
                if (desc)
                {
                    json_p1_set_obis(desc->json_key, buf);
                }               
                break;
            }

            case OBIS_1_0_0_TIMESTAMP:
            {
                extract_timestamp(line);
                break;
            }

            case OBIS_96_14_0_TARIFF:
            {
                const char *start = strchr(line, '(');
                if (start)
                {
                    char buf[8];
                    strncpy(buf, start + 1, 4);
                    buf[4] = '\0';
                    const obis_descriptor_t *desc = get_obis_desc(id);
                    if (desc)
                    {
                        json_p1_set_obis(desc->json_key, buf);
                    }               
                }
                break;
            }

            case OBIS_96_3_10_BREAKER:
            {
                const char *start = strchr(line, '(');
                if (start)
                {
                    char buf[4];
                    strncpy(buf, start + 1, 1);
                    buf[1] = '\0';
                    const obis_descriptor_t *desc = get_obis_desc(id);
                    if (desc)
                    {
                        json_p1_set_obis(desc->json_key, buf);
                    }               
                }
                break;
            }

            default:
                break;
        }

        line = strtok(NULL, "\r\n");
    }

    return data;
}
