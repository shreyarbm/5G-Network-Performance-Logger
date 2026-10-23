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

    (void)remove(path);
    file = fopen(path, "w");
    if (file == NULL || fclose(file) != 0) return puts("FAIL: create empty CSV"), 1;
    if (logger_load(path, loaded, 4U, &count, &malformed) != APP_STATUS_OK || count != 0U) {
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
    if (logger_load(path, loaded, 4U, &count, &malformed) != APP_STATUS_OK ||
        count != 2U || malformed != 1U || loaded[1].timestamp != input.timestamp) {
        puts("FAIL: read valid and malformed rows"); (void)remove(path); return 1;
    }
    if (logger_load("build/does_not_exist.csv", loaded, 4U, &count,
                    &malformed) == APP_STATUS_OK) {
        puts("FAIL: missing file handling"); (void)remove(path); return 1;
    }
    (void)remove(path);
    puts("PASS: empty CSV, append, retrieval, malformed rows, and missing file");
    return 0;
}
