#include <stdio.h>
#include <stdlib.h>

#include "collector.h"

int main(void)
{
    const char *path = "build/test_collector.csv";
    KpiCollector collector;
    KpiHistory history;
    KpiMeasurement measurement;
    size_t malformed_count;
    int is_running;

    (void)remove(path);
    if (collector_init(&collector, path, 1U) != APP_STATUS_OK) {
        puts("FAIL: initialize collector");
        return 1;
    }
    if (collector_get_latest(&collector, &measurement) != APP_STATUS_ERROR ||
        collector_start(&collector) != APP_STATUS_OK ||
        collector_start(&collector) != APP_STATUS_INVALID_INPUT ||
        collector_is_running(&collector, &is_running) != APP_STATUS_OK ||
        is_running == 0) {
        puts("FAIL: start background thread and reject duplicate start");
        (void)collector_destroy(&collector);
        return 1;
    }
    if (collector_wait_for_sample(&collector, 3U) != APP_STATUS_OK ||
        collector_get_latest(&collector, &measurement) != APP_STATUS_OK ||
        kpi_validate(&measurement) != APP_STATUS_OK) {
        puts("FAIL: background thread produces valid sample");
        (void)collector_destroy(&collector);
        return 1;
    }
    if (collector_collect_now(&collector, &measurement) != APP_STATUS_OK ||
        collector_get_latest(&collector, &measurement) != APP_STATUS_OK ||
        collector_stop(&collector) != APP_STATUS_OK ||
        collector_is_running(&collector, &is_running) != APP_STATUS_OK ||
        is_running != 0 ||
        collector_load_history(&collector, &history, &malformed_count) != APP_STATUS_OK ||
        history.count < 2U || malformed_count != 0U ||
        collector_take_status(&collector) != APP_STATUS_OK) {
        puts("FAIL: synchronize collection, CSV access, and stop/join");
        (void)collector_destroy(&collector);
        return 1;
    }
    if (collector_stop(&collector) != APP_STATUS_OK ||
        collector_destroy(&collector) != APP_STATUS_OK) {
        puts("FAIL: clean repeated stop and destroy");
        return 1;
    }
    (void)remove(path);
    puts("PASS: worker creation, synchronized collection, history access, and clean shutdown");
    return 0;
}
