#include <stdio.h>
#include "analysis.h"

int main(void)
{
    KpiMeasurement m[3] = {
        {1, 10.0, 100.0, 0.1, 20.0, 30.0, KPI_SOURCE_SIMULATED},
        {2, 20.0, 200.0, 0.3, 40.0, 50.0, KPI_SOURCE_SIMULATED},
        {3, 30.0, 300.0, 0.5, 60.0, 70.0, KPI_SOURCE_SIMULATED}
    };
    KpiStatistics s;
    size_t invalid;
    if (analysis_calculate(m, 3U, &s, &invalid) != APP_STATUS_OK ||
        s.record_count != 3U || invalid != 0U ||
        s.average_latency_ms != 20.0 || s.minimum_latency_ms != 10.0 ||
        s.maximum_latency_ms != 30.0 || s.average_throughput_mbps != 200.0 ||
        s.minimum_throughput_mbps != 100.0 || s.maximum_throughput_mbps != 300.0 ||
        s.average_packet_loss_percent != 0.3 || s.minimum_packet_loss_percent != 0.1 ||
        s.maximum_packet_loss_percent != 0.5 || s.average_cpu_utilization_percent != 40.0 ||
        s.minimum_cpu_utilization_percent != 20.0 || s.maximum_cpu_utilization_percent != 60.0 ||
        s.average_memory_utilization_percent != 50.0 ||
        s.minimum_memory_utilization_percent != 30.0 ||
        s.maximum_memory_utilization_percent != 70.0) {
        puts("FAIL: deterministic statistics"); return 1;
    }
    if (analysis_calculate(m, 1U, &s, &invalid) != APP_STATUS_OK ||
        s.record_count != 1U || s.minimum_latency_ms != 10.0 ||
        s.maximum_latency_ms != 10.0 || s.average_latency_ms != 10.0) {
        puts("FAIL: one-record statistics"); return 1;
    }
    m[1].latency_ms = -1.0;
    if (analysis_calculate(m, 3U, &s, &invalid) != APP_STATUS_OK ||
        s.record_count != 2U || invalid != 1U) {
        puts("FAIL: invalid measurement skipped"); return 1;
    }
    if (analysis_calculate(NULL, 0U, &s, &invalid) != APP_STATUS_OK || s.record_count != 0U) {
        puts("FAIL: empty dataset"); return 1;
    }
    puts("PASS: analysis averages, extrema, invalid rows, and empty data");
    return 0;
}

