#ifndef THRESHOLDS_H
#define THRESHOLDS_H

#include <stddef.h>
#include "kpi.h"


typedef struct {
    double latency_warning_ms;
    double latency_critical_ms;
    double throughput_warning_mbps;
    double throughput_critical_mbps;
    double packet_loss_warning_percent;
    double packet_loss_critical_percent;
    double cpu_warning_percent;
    double cpu_critical_percent;
    double memory_warning_percent;
    double memory_critical_percent;
} KpiThresholds;

typedef enum {
    KPI_SEVERITY_NORMAL = 0,
    KPI_SEVERITY_WARNING = 1,
    KPI_SEVERITY_CRITICAL = 2
} KpiSeverity;

typedef enum {
    KPI_METRIC_LATENCY = 0,
    KPI_METRIC_THROUGHPUT,
    KPI_METRIC_PACKET_LOSS,
    KPI_METRIC_CPU,
    KPI_METRIC_MEMORY
} KpiMetric;

typedef struct {
    KpiMetric metric;
    KpiSeverity severity;
    double measured_value;
    double threshold_value;
    const char *unit;
} KpiThresholdResult;

#define KPI_THRESHOLD_RESULT_CAPACITY 5U

void thresholds_load_defaults(KpiThresholds *thresholds);
AppStatus thresholds_check(const KpiMeasurement *measurement,
                           const KpiThresholds *thresholds,
                           KpiThresholdResult *results,
                           size_t result_capacity,
                           size_t *result_count);

const char *thresholds_metric_name(KpiMetric metric);
const char *thresholds_severity_name(KpiSeverity severity);

#endif


