#include <stdio.h>
#include "alerts.h"
#include "utils.h"

AppStatus alerts_generate(FILE *output, const KpiMeasurement *measurement,
                          const KpiThresholdResult *results, size_t result_count)
{
    char timestamp_text[32];
    size_t index;
    int violations;

    if (output == NULL || kpi_validate(measurement) != APP_STATUS_OK ||
        results == NULL || result_count != KPI_THRESHOLD_RESULT_CAPACITY) {
        return APP_STATUS_INVALID_INPUT;
    }

    if (utils_format_timestamp(measurement->timestamp, timestamp_text,
                               sizeof(timestamp_text)) != APP_STATUS_OK) {
        return APP_STATUS_ERROR;
    }

    for (index = 0U; index < result_count; ++index) {
        if (results[index].unit == NULL ||
            results[index].severity < KPI_SEVERITY_NORMAL ||
            results[index].severity > KPI_SEVERITY_CRITICAL) {
            return APP_STATUS_INVALID_INPUT;
        }
    }

    violations = 0;
    for (index = 0U; index < result_count; ++index) {
        if (results[index].severity == KPI_SEVERITY_NORMAL) {
            continue;
        }
        violations++;
        if (fprintf(output, "[%s] %s: %s - %.2f %s (threshold %.2f %s)\n",
                    timestamp_text, thresholds_severity_name(results[index].severity),
                    thresholds_metric_name(results[index].metric),
                    results[index].measured_value, results[index].unit,
                    results[index].threshold_value, results[index].unit) < 0) {
            return APP_STATUS_ERROR;
        }
    }

    if (violations == 0 && fprintf(output, "[%s] NORMAL: no threshold violations.\n",
                                   timestamp_text) < 0) {
        return APP_STATUS_ERROR;
    }
    return ferror(output) == 0 ? APP_STATUS_OK : APP_STATUS_ERROR;
}


