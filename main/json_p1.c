#include "json_p1.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static json_obis_entry_t obis_table[OBIS_MAX_COUNT];
static int obis_count = 0;

static json_peak_entry_t peak_history[PEAK_HISTORY_MAX];
static int peak_count = 0;

void json_p1_reset(void)
{
    obis_count = 0;
    peak_count = 0;
}

void json_p1_set_obis(const char *obis, const char *value)
{
    if (!obis || !value)
    {
        return;
    }

    // Update existing entry
    for (int i = 0; i < obis_count; i++)
    {
        if (strcmp(obis_table[i].obis, obis) == 0)
        {
            strncpy(obis_table[i].value, value,
                    sizeof(obis_table[i].value) - 1);
            obis_table[i].value[sizeof(obis_table[i].value) - 1] = '\0';
            return;
        }
    }

    // Add new entry
    if (obis_count < OBIS_MAX_COUNT)
    {
        strncpy(obis_table[obis_count].obis, obis,
                sizeof(obis_table[obis_count].obis) - 1);
        obis_table[obis_count].obis[sizeof(obis_table[obis_count].obis) - 1] = '\0';

        strncpy(obis_table[obis_count].value, value,
                sizeof(obis_table[obis_count].value) - 1);
        obis_table[obis_count].value[sizeof(obis_table[obis_count].value) - 1] = '\0';

        obis_count++;
    }
}

void json_p1_add_peak_event(const char *timestamp, const char *value)
{
    if (!timestamp || !value)
    {
        return;
    }

    if (peak_count < PEAK_HISTORY_MAX)
    {
        strncpy(peak_history[peak_count].timestamp, timestamp,
                sizeof(peak_history[peak_count].timestamp) - 1);
        peak_history[peak_count].timestamp[sizeof(peak_history[peak_count].timestamp) - 1] = '\0';

        strncpy(peak_history[peak_count].value, value,
                sizeof(peak_history[peak_count].value) - 1);
        peak_history[peak_count].value[sizeof(peak_history[peak_count].value) - 1] = '\0';

        peak_count++;
    }
}

char *json_p1_build(bool *is_full)
{
    if (is_full)
    {
        *is_full = true;
    }

    char *json = malloc(4096);
    if (!json)
    {
        return NULL;
    }

    size_t offset = 0;
    json[offset++] = '{';

    // OBIS entries
    for (int i = 0; i < obis_count; i++)
    {
        offset += snprintf(json + offset, 4096 - offset,
                           "\"%s\":\"%s\",",
                           obis_table[i].obis,
                           obis_table[i].value);
    }

    // Peak history
    if (peak_count > 0)
    {
        offset += snprintf(json + offset, 4096 - offset, "\"1.6.0_hist\":[");

        for (int i = 0; i < peak_count; i++)
        {
            offset += snprintf(json + offset, 4096 - offset,
                               "{\"ts\":\"%s\",\"val\":\"%s\"},",
                               peak_history[i].timestamp,
                               peak_history[i].value);
        }

        if (json[offset - 1] == ',')
        {
            offset--;
        }

        offset += snprintf(json + offset, 4096 - offset, "],");
    }

    if (json[offset - 1] == ',')
    {
        offset--;
    }

    json[offset++] = '}';
    json[offset] = '\0';

    return json;
}
