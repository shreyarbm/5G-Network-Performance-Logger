#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stddef.h>
#include "kpi.h"

typedef struct {
    size_t record_count;
    double average_latency_ms;
    double minimum_latency_ms;
    double maximum_latency_ms;
    double average_throughput_mbps;
    double minimum_throughput_mbps;
    double maximum_throughput_mbps;
    double average_packet_loss_percent;
    double minimum_packet_loss_percent;
    double maximum_packet_loss_percent;
    double average_cpu_utilization_percent;
    double minimum_cpu_utilization_percent;
    double maximum_cpu_utilization_percent;
    double average_memory_utilization_percent;
    double minimum_memory_utilization_percent;
    double maximum_memory_utilization_percent;
} KpiStatistics;


AppStatus analysis_calculate(const KpiMeasurement *records, size_t count,
                             KpiStatistics *statistics,
                             size_t *invalid_count);

#endif

