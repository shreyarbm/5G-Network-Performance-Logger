#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "kpi.h"

AppStatus kpi_validate(const KpiMeasurement *measurement)
{
    if (measurement == NULL || measurement->timestamp == (time_t)-1) {
        return APP_STATUS_INVALID_INPUT;
    }

    if (!isfinite(measurement->latency_ms) ||
        !isfinite(measurement->throughput_mbps) ||
        !isfinite(measurement->packet_loss_percent) ||
        !isfinite(measurement->cpu_utilization_percent) ||
        !isfinite(measurement->memory_utilization_percent) ||
        measurement->latency_ms < KPI_LATENCY_MIN_MS ||
        measurement->latency_ms > KPI_LATENCY_MAX_MS ||
        measurement->throughput_mbps < KPI_THROUGHPUT_MIN_MBPS ||
        measurement->throughput_mbps > KPI_THROUGHPUT_MAX_MBPS ||
        measurement->packet_loss_percent < KPI_PERCENT_MIN ||
        measurement->packet_loss_percent > KPI_PERCENT_MAX ||
        measurement->cpu_utilization_percent < KPI_PERCENT_MIN ||
        measurement->cpu_utilization_percent > KPI_PERCENT_MAX ||
        measurement->memory_utilization_percent < KPI_PERCENT_MIN ||
        measurement->memory_utilization_percent > KPI_PERCENT_MAX ||
        measurement->source != KPI_SOURCE_SIMULATED) {
        return APP_STATUS_INVALID_INPUT;
    }

    return APP_STATUS_OK;
}

AppStatus kpi_collect_simulated(KpiMeasurement *measurement)
{
    time_t current_time;

    if (measurement == NULL) {
        return APP_STATUS_INVALID_INPUT;
    }

    current_time = time(NULL);
    if (current_time == (time_t)-1) {
        return APP_STATUS_ERROR;
    }

    measurement->timestamp = current_time;
    measurement->latency_ms = 5.0 + (double)(rand() % 196);
    measurement->throughput_mbps = 50.0 + (double)(rand() % 951);
    measurement->packet_loss_percent = (double)(rand() % 501) / 100.0;
    measurement->cpu_utilization_percent = (double)(rand() % 101);
    measurement->memory_utilization_percent = (double)(rand() % 101);
    measurement->source = KPI_SOURCE_SIMULATED;

    return kpi_validate(measurement);
}
