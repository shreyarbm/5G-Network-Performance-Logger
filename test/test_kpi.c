#include <math.h>
#include <stdio.h>
#include "kpi.h"

int main(void)
{
    KpiMeasurement m = {1, 0.0, 0.0, 0.0, 0.0, 100.0, KPI_SOURCE_SIMULATED};
    KpiMeasurement generated;

    if (kpi_validate(&m) != APP_STATUS_OK) return puts("FAIL: boundary values"), 1;
    m.latency_ms = -0.01;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: negative latency"), 1;
    m.latency_ms = 1.0; m.throughput_mbps = -0.01;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: negative throughput"), 1;
    m.throughput_mbps = 10001.0;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: throughput maximum"), 1;
    m.throughput_mbps = 10.0; m.packet_loss_percent = 100.1;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: packet loss range"), 1;
    m.packet_loss_percent = 0.0; m.cpu_utilization_percent = -1.0;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: CPU range"), 1;
    m.cpu_utilization_percent = 0.0; m.memory_utilization_percent = 101.0;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: memory range"), 1;
    m.memory_utilization_percent = NAN;
    if (kpi_validate(&m) == APP_STATUS_OK) return puts("FAIL: non-finite value"), 1;

    if (kpi_collect_simulated(&generated) != APP_STATUS_OK ||
        generated.source != KPI_SOURCE_SIMULATED) {
        puts("FAIL: simulated collection"); return 1;
    }
    puts("PASS: KPI boundaries, invalid ranges, and simulated collection");
    return 0;
}
