#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include "kpi.h"

#define LOGGER_DEFAULT_PATH "data/kpi_log.csv"
#define LOGGER_CSV_HEADER "timestamp,latency_ms,throughput_mbps,packet_loss_percent,cpu_utilization_percent,memory_utilization_percent,source"
#define LOGGER_HISTORY_CAPACITY 1000U

typedef struct {
    KpiMeasurement records[LOGGER_HISTORY_CAPACITY];
    size_t next_write_index;
    size_t count;
} KpiHistory;

void logger_history_init(KpiHistory *history);
AppStatus logger_history_add(KpiHistory *history,
                             const KpiMeasurement *measurement);
const KpiMeasurement *logger_history_get(const KpiHistory *history,
                                         size_t chronological_index);
AppStatus logger_history_copy(const KpiHistory *history,
                              KpiMeasurement *records, size_t capacity,
                              size_t *record_count);
AppStatus logger_history_load(const char *path, KpiHistory *history,
                              size_t *malformed_count);


AppStatus logger_append(const char *path, const KpiMeasurement *measurement);


#endif

