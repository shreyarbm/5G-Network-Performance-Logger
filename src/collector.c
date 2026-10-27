#include <errno.h>
#include <string.h>
#include <time.h>

#include "collector.h"
#include "kpi.h"

static AppStatus collector_collect_locked(KpiCollector *collector,
                                          KpiMeasurement *measurement)
{
    KpiMeasurement collected;
    AppStatus status;

    status = kpi_collect_simulated(&collected);
    if (status == APP_STATUS_OK) {
        status = logger_append(collector->log_path, &collected);
    }
    if (status == APP_STATUS_OK) {
        collector->latest = collected;
        collector->has_latest = 1;
        if (measurement != NULL) {
            *measurement = collected;
        }
    }
    collector->last_status = status;
    (void)pthread_cond_broadcast(&collector->condition);
    return status;
}

static void *collector_thread(void *argument)
{
    KpiCollector *collector = argument;

    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return NULL;
    }
    while (collector->stop_requested == 0) {
        struct timespec deadline;
        int wait_status;

        if (timespec_get(&deadline, TIME_UTC) == 0) {
            collector->last_status = APP_STATUS_ERROR;
            break;
        }
        deadline.tv_sec += (time_t)collector->interval_seconds;
        wait_status = pthread_cond_timedwait(&collector->condition,
                                             &collector->mutex, &deadline);
        if (collector->stop_requested != 0) {
            break;
        }
        if (wait_status == ETIMEDOUT) {
            (void)collector_collect_locked(collector, NULL);
        } else if (wait_status != 0) {
            collector->last_status = APP_STATUS_ERROR;
            break;
        }
    }
    (void)pthread_mutex_unlock(&collector->mutex);
    return NULL;
}

AppStatus collector_init(KpiCollector *collector, const char *log_path,
                         unsigned int interval_seconds)
{
    size_t path_length;

    if (collector == NULL || log_path == NULL || log_path[0] == '\0' ||
        interval_seconds == 0U) {
        return APP_STATUS_INVALID_INPUT;
    }
    path_length = strlen(log_path);
    if (path_length >= COLLECTOR_PATH_CAPACITY) {
        return APP_STATUS_INVALID_INPUT;
    }

    memset(collector, 0, sizeof(*collector));
    if (pthread_mutex_init(&collector->mutex, NULL) != 0) {
        return APP_STATUS_ERROR;
    }
    if (pthread_cond_init(&collector->condition, NULL) != 0) {
        (void)pthread_mutex_destroy(&collector->mutex);
        return APP_STATUS_ERROR;
    }
    memcpy(collector->log_path, log_path, path_length + 1U);
    collector->interval_seconds = interval_seconds;
    collector->last_status = APP_STATUS_OK;
    collector->initialized = 1;
    return APP_STATUS_OK;
}

AppStatus collector_start(KpiCollector *collector)
{
    if (collector == NULL || collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    if (collector->running != 0) {
        (void)pthread_mutex_unlock(&collector->mutex);
        return APP_STATUS_INVALID_INPUT;
    }

    collector->stop_requested = 0;
    collector->last_status = APP_STATUS_OK;
    collector->running = 1;
    if (pthread_create(&collector->thread, NULL, collector_thread,
                       collector) != 0) {
        collector->running = 0;
        (void)pthread_mutex_unlock(&collector->mutex);
        return APP_STATUS_ERROR;
    }
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        (void)pthread_join(collector->thread, NULL);
        collector->running = 0;
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}

AppStatus collector_stop(KpiCollector *collector)
{
    if (collector == NULL || collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    if (collector->running == 0) {
        (void)pthread_mutex_unlock(&collector->mutex);
        return APP_STATUS_OK;
    }
    collector->stop_requested = 1;
    (void)pthread_cond_signal(&collector->condition);
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    if (pthread_join(collector->thread, NULL) != 0) {
        return APP_STATUS_ERROR;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    collector->running = 0;
    collector->stop_requested = 0;
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}

AppStatus collector_destroy(KpiCollector *collector)
{
    AppStatus status;

    if (collector == NULL || collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    status = collector_stop(collector);
    if (status != APP_STATUS_OK) {
        return status;
    }
    if (pthread_cond_destroy(&collector->condition) != 0 ||
        pthread_mutex_destroy(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    collector->initialized = 0;
    return APP_STATUS_OK;
}

AppStatus collector_collect_now(KpiCollector *collector,
                                KpiMeasurement *measurement)
{
    AppStatus status;

    if (collector == NULL || measurement == NULL ||
        collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    status = collector_collect_locked(collector, measurement);
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return status;
}

AppStatus collector_get_latest(KpiCollector *collector,
                               KpiMeasurement *measurement)
{
    AppStatus status;

    if (collector == NULL || measurement == NULL ||
        collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    status = collector->has_latest != 0 ? APP_STATUS_OK : APP_STATUS_ERROR;
    if (status == APP_STATUS_OK) {
        *measurement = collector->latest;
    }
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return status;
}

AppStatus collector_set_latest(KpiCollector *collector,
                               const KpiMeasurement *measurement)
{
    if (collector == NULL || measurement == NULL ||
        collector->initialized == 0 ||
        kpi_validate(measurement) != APP_STATUS_OK) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    collector->latest = *measurement;
    collector->has_latest = 1;
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}

AppStatus collector_load_history(KpiCollector *collector, KpiHistory *history,
                                 size_t *malformed_count)
{
    AppStatus status;

    if (collector == NULL || history == NULL || malformed_count == NULL ||
        collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    status = logger_history_load(collector->log_path, history, malformed_count);
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return status;
}

AppStatus collector_wait_for_sample(KpiCollector *collector,
                                    unsigned int timeout_seconds)
{
    struct timespec deadline;
    AppStatus status;

    if (collector == NULL || collector->initialized == 0 ||
        timeout_seconds == 0U) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (timespec_get(&deadline, TIME_UTC) == 0) {
        return APP_STATUS_ERROR;
    }
    deadline.tv_sec += (time_t)timeout_seconds;
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    while (collector->has_latest == 0 && collector->running != 0 &&
           collector->stop_requested == 0) {
        int wait_status;

        wait_status = pthread_cond_timedwait(&collector->condition,
                                             &collector->mutex, &deadline);
        if (wait_status == ETIMEDOUT) {
            (void)pthread_mutex_unlock(&collector->mutex);
            return APP_STATUS_ERROR;
        }
        if (wait_status != 0) {
            (void)pthread_mutex_unlock(&collector->mutex);
            return APP_STATUS_ERROR;
        }
    }
    status = collector->has_latest != 0 ? APP_STATUS_OK : APP_STATUS_ERROR;
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return status;
}

AppStatus collector_take_status(KpiCollector *collector)
{
    AppStatus status;

    if (collector == NULL || collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    status = collector->last_status;
    collector->last_status = APP_STATUS_OK;
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return status;
}

AppStatus collector_is_running(KpiCollector *collector, int *is_running)
{
    if (collector == NULL || is_running == NULL ||
        collector->initialized == 0) {
        return APP_STATUS_INVALID_INPUT;
    }
    if (pthread_mutex_lock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    *is_running = collector->running;
    if (pthread_mutex_unlock(&collector->mutex) != 0) {
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}
