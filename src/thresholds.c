#include <math.h>
#include <stddef.h>

#include "thresholds.h"

void thresholds_load_defaults(KpiThresholds *thresholds)
{
    if (thresholds == NULL) {
        return;
    }

    thresholds->latency_warning_ms = 100.0;
    thresholds->latency_critical_ms = 200.0;
    thresholds->throughput_warning_mbps = 100.0;
    thresholds->throughput_critical_mbps = 50.0;
    thresholds->packet_loss_warning_percent = 1.0;
    thresholds->packet_loss_critical_percent = 3.0;
    thresholds->cpu_warning_percent = 80.0;
    thresholds->cpu_critical_percent = 95.0;
    thresholds->memory_warning_percent = 80.0;
    thresholds->memory_critical_percent = 95.0;
}

static int thresholds_are_valid(const KpiThresholds *thresholds)
{
    return isfinite(thresholds->latency_warning_ms) &&
           isfinite(thresholds->latency_critical_ms) &&
           isfinite(thresholds->throughput_warning_mbps) &&
           isfinite(thresholds->throughput_critical_mbps) &&
           isfinite(thresholds->packet_loss_warning_percent) &&
           isfinite(thresholds->packet_loss_critical_percent) &&
           isfinite(thresholds->cpu_warning_percent) &&
           isfinite(thresholds->cpu_critical_percent) &&
           isfinite(thresholds->memory_warning_percent) &&
           isfinite(thresholds->memory_critical_percent) &&
           thresholds->latency_warning_ms >= KPI_LATENCY_MIN_MS &&
           thresholds->latency_critical_ms <= KPI_LATENCY_MAX_MS &&
           thresholds->latency_warning_ms <= thresholds->latency_critical_ms &&
           thresholds->throughput_critical_mbps >= KPI_THROUGHPUT_MIN_MBPS &&
           thresholds->throughput_warning_mbps <= KPI_THROUGHPUT_MAX_MBPS &&
           thresholds->throughput_critical_mbps <= thresholds->throughput_warning_mbps &&
           thresholds->packet_loss_warning_percent >= KPI_PERCENT_MIN &&
           thresholds->packet_loss_critical_percent <= KPI_PERCENT_MAX &&
           thresholds->packet_loss_warning_percent <= thresholds->packet_loss_critical_percent &&
           thresholds->cpu_warning_percent >= KPI_PERCENT_MIN &&
           thresholds->cpu_critical_percent <= KPI_PERCENT_MAX &&
           thresholds->cpu_warning_percent <= thresholds->cpu_critical_percent &&
           thresholds->memory_warning_percent >= KPI_PERCENT_MIN &&
           thresholds->memory_critical_percent <= KPI_PERCENT_MAX &&
           thresholds->memory_warning_percent <= thresholds->memory_critical_percent;
}

static void thresholds_evaluate(KpiThresholdResult *result, KpiMetric metric,
                                double value, double warning, double critical,
                                int high_is_bad, const char *unit)
{
    result->metric = metric;
    result->measured_value = value;
    result->unit = unit;
    result->severity = KPI_SEVERITY_NORMAL;
    result->threshold_value = warning;

    if (high_is_bad != 0) {
        if (value >= critical) {
            result->severity = KPI_SEVERITY_CRITICAL;
            result->threshold_value = critical;
        } else if (value >= warning) {
            result->severity = KPI_SEVERITY_WARNING;
        }
    } else {
        if (value <= critical) {
            result->severity = KPI_SEVERITY_CRITICAL;
            result->threshold_value = critical;
        } else if (value <= warning) {
            result->severity = KPI_SEVERITY_WARNING;
        }
    }
}

AppStatus thresholds_check(const KpiMeasurement *measurement,
                           const KpiThresholds *thresholds,
                           KpiThresholdResult *results,
                           size_t result_capacity,
                           size_t *result_count)
{
    if (measurement == NULL || thresholds == NULL || results == NULL ||
        result_count == NULL || result_capacity < KPI_THRESHOLD_RESULT_CAPACITY ||
        kpi_validate(measurement) != APP_STATUS_OK ||
        !thresholds_are_valid(thresholds)) {
        return APP_STATUS_INVALID_INPUT;
    }

    thresholds_evaluate(&results[0], KPI_METRIC_LATENCY, measurement->latency_ms,
                        thresholds->latency_warning_ms, thresholds->latency_critical_ms,
                        1, "ms");
    thresholds_evaluate(&results[1], KPI_METRIC_THROUGHPUT, measurement->throughput_mbps,
                        thresholds->throughput_warning_mbps,
                        thresholds->throughput_critical_mbps, 0, "Mbps");
    thresholds_evaluate(&results[2], KPI_METRIC_PACKET_LOSS,
                        measurement->packet_loss_percent,
                        thresholds->packet_loss_warning_percent,
                        thresholds->packet_loss_critical_percent, 1, "%");
    thresholds_evaluate(&results[3], KPI_METRIC_CPU,
                        measurement->cpu_utilization_percent,
                        thresholds->cpu_warning_percent, thresholds->cpu_critical_percent,
                        1, "%");
    thresholds_evaluate(&results[4], KPI_METRIC_MEMORY,
                        measurement->memory_utilization_percent,
                        thresholds->memory_warning_percent,
                        thresholds->memory_critical_percent, 1, "%");
    *result_count = KPI_THRESHOLD_RESULT_CAPACITY;
    return APP_STATUS_OK;
}

const char *thresholds_metric_name(KpiMetric metric)
{
    switch (metric) {
        case KPI_METRIC_LATENCY: return "Latency";
        case KPI_METRIC_THROUGHPUT: return "Throughput";
        case KPI_METRIC_PACKET_LOSS: return "Packet loss";
        case KPI_METRIC_CPU: return "CPU utilization";
        case KPI_METRIC_MEMORY: return "Memory utilization";
        default: return "Unknown KPI";
    }
}

const char *thresholds_severity_name(KpiSeverity severity)
{
    switch (severity) {
        case KPI_SEVERITY_NORMAL: return "NORMAL";
        case KPI_SEVERITY_WARNING: return "WARNING";
        case KPI_SEVERITY_CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

