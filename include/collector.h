#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <pthread.h>
#include <stddef.h>

#include "logger.h"

#define COLLECTOR_PATH_CAPACITY 256U
#define COLLECTOR_DEFAULT_INTERVAL_SECONDS 5U

typedef struct {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    char log_path[COLLECTOR_PATH_CAPACITY];
    unsigned int interval_seconds;
    int initialized;
    int running;
    int stop_requested;
    int has_latest;
    AppStatus last_status;
    KpiMeasurement latest;
} KpiCollector;

AppStatus collector_init(KpiCollector *collector, const char *log_path,
                         unsigned int interval_seconds);
AppStatus collector_start(KpiCollector *collector);
AppStatus collector_stop(KpiCollector *collector);
AppStatus collector_destroy(KpiCollector *collector);
AppStatus collector_collect_now(KpiCollector *collector,
                                KpiMeasurement *measurement);
AppStatus collector_get_latest(KpiCollector *collector,
                               KpiMeasurement *measurement);
AppStatus collector_set_latest(KpiCollector *collector,
                               const KpiMeasurement *measurement);
AppStatus collector_load_history(KpiCollector *collector, KpiHistory *history,
                                 size_t *malformed_count);
AppStatus collector_wait_for_sample(KpiCollector *collector,
                                    unsigned int timeout_seconds);
AppStatus collector_take_status(KpiCollector *collector);
AppStatus collector_is_running(KpiCollector *collector, int *is_running);

#endif
