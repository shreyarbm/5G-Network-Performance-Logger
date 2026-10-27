#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

int main(void)
{
    const char *path = "build/test_kpi.csv";
    KpiMeasurement input = {1234, 20.0, 200.0, 0.2, 30.0, 40.0, KPI_SOURCE_SIMULATED};
    KpiMeasurement loaded[4];
    size_t count = 0U, malformed = 0U;
    FILE *file;
    KpiHistory history;
    size_t index;
    const char *large_path = "build/test_history.csv";

    (void)remove(path);
    file = fopen(path, "w");
    if (file == NULL || fclose(file) != 0) return puts("FAIL: create empty CSV"), 1;
    if (logger_history_load(path, &history, &malformed) != APP_STATUS_OK ||
        history.count != 0U) {
        puts("FAIL: empty CSV read"); (void)remove(path); return 1;
    }
    if (logger_append(path, &input) != APP_STATUS_OK ||
        logger_append(path, &input) != APP_STATUS_OK) {
        puts("FAIL: append records"); (void)remove(path); return 1;
    }
    file = fopen(path, "a");
    if (file == NULL) return puts("FAIL: open test CSV"), 1;
    (void)fputs("malformed,row\n", file);
    if (fclose(file) != 0) return puts("FAIL: close test CSV"), 1;
    if (logger_history_load(path, &history, &malformed) != APP_STATUS_OK ||
        history.count != 2U || malformed != 1U ||
        logger_history_copy(&history, loaded, 4U, &count) != APP_STATUS_OK ||
        count != 2U || loaded[1].timestamp != input.timestamp) {
        puts("FAIL: read valid and malformed rows"); (void)remove(path); return 1;
    }
    if (logger_append("build/no_logger_test_dir/output.csv", &input) == APP_STATUS_OK) {
        puts("FAIL: write path error handling"); (void)remove(path); return 1;
    }
    if (logger_history_load("build/does_not_exist.csv", &history,
                            &malformed) == APP_STATUS_OK) {
        puts("FAIL: missing file handling"); (void)remove(path); return 1;
    }
    (void)remove(path);

    logger_history_init(&history);
    if (history.count != 0U || logger_history_get(&history, 0U) != NULL) {
        puts("FAIL: empty circular history"); return 1;
    }
    if (logger_history_add(&history, &input) != APP_STATUS_OK ||
        history.count != 1U || logger_history_get(&history, 0U)->timestamp != input.timestamp) {
        puts("FAIL: one-record circular history"); return 1;
    }
    logger_history_init(&history);
    for (index = 0U; index < LOGGER_HISTORY_CAPACITY; ++index) {
        KpiMeasurement item = input;
        item.timestamp = (time_t)(index + 1U);
        if (logger_history_add(&history, &item) != APP_STATUS_OK) {
            puts("FAIL: fill circular history"); return 1;
        }
    }
    if (history.count != LOGGER_HISTORY_CAPACITY ||
        logger_history_get(&history, 0U)->timestamp != (time_t)1 ||
        logger_history_get(&history, LOGGER_HISTORY_CAPACITY - 1U)->timestamp !=
            (time_t)LOGGER_HISTORY_CAPACITY) {
        puts("FAIL: exactly full circular history"); return 1;
    }
    {
        KpiMeasurement item = input;
        KpiMeasurement ordered[LOGGER_HISTORY_CAPACITY];
        item.timestamp = (time_t)(LOGGER_HISTORY_CAPACITY + 1U);
        if (logger_history_add(&history, &item) != APP_STATUS_OK ||
            history.count != LOGGER_HISTORY_CAPACITY ||
            logger_history_get(&history, 0U)->timestamp != (time_t)2 ||
            logger_history_get(&history, LOGGER_HISTORY_CAPACITY - 1U)->timestamp !=
                (time_t)(LOGGER_HISTORY_CAPACITY + 1U) ||
            logger_history_copy(&history, ordered, LOGGER_HISTORY_CAPACITY,
                                &count) != APP_STATUS_OK || count != LOGGER_HISTORY_CAPACITY ||
            ordered[0].timestamp != (time_t)2 ||
            ordered[LOGGER_HISTORY_CAPACITY - 1U].timestamp !=
                (time_t)(LOGGER_HISTORY_CAPACITY + 1U)) {
            puts("FAIL: wraparound and chronological order"); return 1;
        }
    }

    file = fopen(large_path, "w");
    if (file == NULL || fprintf(file, "%s\n", LOGGER_CSV_HEADER) < 0) {
        if (file != NULL) (void)fclose(file);
        puts("FAIL: create large CSV"); return 1;
    }
    for (index = 0U; index < LOGGER_HISTORY_CAPACITY + 5U; ++index) {
        if (index == 10U && fputs("broken,csv,row\n", file) == EOF) {
            (void)fclose(file); (void)remove(large_path);
            puts("FAIL: write malformed CSV row"); return 1;
        }
        if (fprintf(file, "%lu,20.00,200.00,0.20,30.00,40.00,SIMULATED\n",
                    (unsigned long)(index + 1U)) < 0) {
            (void)fclose(file); (void)remove(large_path);
            puts("FAIL: write large CSV rows"); return 1;
        }
    }
    if (fclose(file) != 0 ||
        logger_history_load(large_path, &history, &malformed) != APP_STATUS_OK ||
        history.count != LOGGER_HISTORY_CAPACITY || malformed != 1U ||
        logger_history_get(&history, 0U)->timestamp != (time_t)6 ||
        logger_history_get(&history, LOGGER_HISTORY_CAPACITY - 1U)->timestamp !=
            (time_t)(LOGGER_HISTORY_CAPACITY + 5U)) {
        (void)remove(large_path);
        puts("FAIL: load large CSV and retain latest valid records"); return 1;
    }
    (void)remove(large_path);
    puts("PASS: empty CSV, append, retrieval, malformed rows, and file errors");
    return 0;
}
