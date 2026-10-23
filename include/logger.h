#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include "kpi.h"

#define LOGGER_DEFAULT_PATH "data/kpi_log.csv"
#define LOGGER_CSV_HEADER "timestamp,latency_ms,throughput_mbps,packet_loss_percent,cpu_utilization_percent,memory_utilization_percent,source"


AppStatus logger_append(const char *path, const KpiMeasurement *measurement);


AppStatus logger_load(const char *path, KpiMeasurement *records,
                      size_t capacity, size_t *record_count,
                      size_t *malformed_count);

#endif

