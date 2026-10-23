#ifndef KPI_H
#define KPI_H

#include <time.h>
#include "common.h"

#define KPI_LATENCY_MIN_MS 0.0
#define KPI_LATENCY_MAX_MS 1000.0
#define KPI_THROUGHPUT_MIN_MBPS 0.0
#define KPI_THROUGHPUT_MAX_MBPS 10000.0
#define KPI_PERCENT_MIN 0.0
#define KPI_PERCENT_MAX 100.0

typedef enum {
    KPI_SOURCE_SIMULATED = 0
} KpiSource;

typedef struct {
    time_t timestamp;
    double latency_ms;
    double throughput_mbps;
    double packet_loss_percent;
    double cpu_utilization_percent;
    double memory_utilization_percent;
    KpiSource source;
} KpiMeasurement;


AppStatus kpi_validate(const KpiMeasurement *measurement);


AppStatus kpi_collect_simulated(KpiMeasurement *measurement);

#endif

