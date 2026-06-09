#include "dsmr_parser.h"

#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "gvc_dsmr_parser";

MeterFeatures detect_obis_features(const char *telegram)
{
    MeterFeatures f = {0};

    if (strstr(telegram, "52.7.0") || strstr(telegram, "72.7.0"))
        f.isThreePhase = true;

    if (strstr(telegram, "0-1:24.2.1"))
        f.hasGas = true;

    if (strstr(telegram, "0-2:24.2.3"))
        f.hasWater = true;

    if (strstr(telegram, "0-3:24.2.4"))
        f.hasHeat = true;

    if (strstr(telegram, "1-0:2.7.0"))
        f.hasSolar = true;
        
    if (strstr(telegram, "1-0:1.6.0"))
        f.hasMonthlyPeak = true;

    return f;
}

static float extract_float(const char *line)
{
    const char *start = strchr(line, '(');
    const char *end   = strchr(line, '*');   // DSMR uses *kWh, *V, *A, etc.

    if (!start || !end || end <= start)
        return 0.0f;

    char buf[32];
    size_t len = end - (start + 1);
    if (len >= sizeof(buf))
        len = sizeof(buf) - 1;

    memcpy(buf, start + 1, len);
    buf[len] = '\0';

    return atof(buf);
}

dsmr_data_t dsmr_parse(const char *telegram)
{
    dsmr_data_t data = {0};

    data.features = detect_obis_features(telegram);

    char *copy = strdup(telegram);
    if (!copy)
        return data;

    char *line = strtok(copy, "\r\n");
    while (line) {

        // ---------------------------------------------------------------------------
        // ELECTRICITY (1‑phase and 3‑phase)
        // DSMR/SMR 5.x standard OBIS codes
        //
        // 1‑0:1.8.0  → total import (kWh)
        // 1‑0:2.8.0  → total export (kWh)
        //
        // 1‑0:32.7.0 → voltage L1 (V)
        // 1‑0:52.7.0 → voltage L2 (V)
        // 1‑0:72.7.0 → voltage L3 (V)
        //
        // 1‑0:31.7.0 → current L1 (A)
        // 1‑0:51.7.0 → current L2 (A)
        // 1‑0:71.7.0 → current L3 (A)
        // ---------------------------------------------------------------------------

        // 1-0:1.8.0 = total import kWh
        if (strstr(line, "1-0:1.8.0")) {
            data.power_import = extract_float(line);
        }

        // 1-0:2.8.0 = total export kWh
        else if (strstr(line, "1-0:2.8.0")) {
            data.power_export = extract_float(line);
        }

        // 1-0:32.7.0 = voltage L1
        else if (strstr(line, "1-0:32.7.0")) {
            data.voltage_l1 = extract_float(line);
        }

        // 1-0:31.7.0 = current L1
        else if (strstr(line, "1-0:31.7.0")) {
            data.current_l1 = extract_float(line);
        }
        
        // 1-0:32.7.0 = voltage L1
        else if (strstr(line, "1-0:32.7.0")) {
            data.voltage_l1 = extract_float(line);
        }

        // 1-0:52.7.0 = voltage L2
        else if (strstr(line, "1-0:52.7.0")) {
            data.voltage_l2 = extract_float(line);
        }

        // 1-0:72.7.0 = voltage L3
        else if (strstr(line, "1-0:72.7.0")) {
            data.voltage_l3 = extract_float(line);
        }

        // 1-0:31.7.0 = current L1
        else if (strstr(line, "1-0:31.7.0")) {
            data.current_l1 = extract_float(line);
        }

        // 1-0:51.7.0 = current L2
        else if (strstr(line, "1-0:51.7.0")) {
            data.current_l2 = extract_float(line);
        }

        // 1-0:71.7.0 = current L3
        else if (strstr(line, "1-0:71.7.0")) {
            data.current_l3 = extract_float(line);
        }

        // ---------------------------------------------------------------------------
        // MONTHLY PEAK DEMAND (capacity tariff)
        // 1-0:1.6.0 = highest 15-minute average power of the current month (kW)
        // ---------------------------------------------------------------------------
        else if (strstr(line, "1-0:1.6.0")) {
            data.monthly_peak_kw = extract_float(line);
        }

        // ---------------------------------------------------------------------------
        // GAS: 0-1:24.2.1
        // Format example: 0-1:24.2.1(240609190000S)(00123.456*m3)
        // We want the second (...) which contains the value.
        // ---------------------------------------------------------------------------
        else if (strstr(line, "0-1:24.2.1")) {
            const char *second = strchr(line, ')');   // end of timestamp
            if (second) {
                second = strchr(second + 1, '(');     // start of value
                if (second) {
                    data.gas_m3 = atof(second + 1);
                }
            }
        }

        // ---------------------------------------------------------------------------
        // WATER: 0-2:24.2.3
        // Format example: 0-2:24.2.3(240609190000S)(00456.789*m3)
        // Same structure as gas.
        // ---------------------------------------------------------------------------
        else if (strstr(line, "0-2:24.2.3")) {
            const char *second = strchr(line, ')');
            if (second) {
                second = strchr(second + 1, '(');
                if (second) {
                    data.water_m3 = atof(second + 1);
                }
            }
        }

        // ---------------------------------------------------------------------------
        // HEAT: 0-3:24.2.4
        // Format example: 0-3:24.2.4(240609190000S)(012.345*GJ)
        // Same structure again.
        // ---------------------------------------------------------------------------
        else if (strstr(line, "0-3:24.2.4")) {
            const char *second = strchr(line, ')');
            if (second) {
                second = strchr(second + 1, '(');
                if (second) {
                    data.heat_gj = atof(second + 1);
                }
            }
        }

        // ---------------------------------------------------------------------------
        // SOLAR: 1-0:2.7.0
        // Format example: 1-0:2.7.0(240609190000S)(012.345*kWh)
        // Same structure again.
        // ---------------------------------------------------------------------------
        else if (strstr(line, "1-0:2.7.0")) {
            const char *second = strchr(line, ')');
            if (second) {
                second = strchr(second + 1, '(');
                if (second) {
                    data.solar_kwh = atof(second + 1);
                }
            }
        }

        line = strtok(NULL, "\r\n");
    }

    free(copy);
    ESP_LOGI(TAG, "Parsed DSMR: import=%.3f, export=%.3f, U=%.1fV, I=%.1fA",
             data.power_import, data.power_export,
             data.voltage_l1, data.current_l1);

    return data;
}
