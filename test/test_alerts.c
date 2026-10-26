#include <stdio.h>
#include <string.h>
#include "alerts.h"

int main(void)
{
    KpiMeasurement m = {1700000000, 99.99, 100.01, 0.99, 79.99, 79.99,
                        KPI_SOURCE_SIMULATED};
    KpiThresholds thresholds;
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY];
    size_t count;
    FILE *stream;
    char output[512];
    size_t bytes;
    size_t index;

    thresholds_load_defaults(&thresholds);
    thresholds.latency_warning_ms = 300.0;
    if (thresholds_check(&m, &thresholds, results, KPI_THRESHOLD_RESULT_CAPACITY,
                         &count) == APP_STATUS_OK) {
        puts("FAIL: invalid threshold configuration"); return 1;
    }
    thresholds_load_defaults(&thresholds);
    if (thresholds_check(&m, &thresholds, results, KPI_THRESHOLD_RESULT_CAPACITY,
                         &count) != APP_STATUS_OK) {
        puts("FAIL: normal threshold check"); return 1;
    }
    for (index = 0U; index < count; ++index) {
        if (results[index].severity != KPI_SEVERITY_NORMAL) {
            puts("FAIL: below warning thresholds"); return 1;
        }
    }

    m.latency_ms = 100.0; m.throughput_mbps = 100.0;
    m.packet_loss_percent = 1.0; m.cpu_utilization_percent = 80.0;
    m.memory_utilization_percent = 80.0;
    if (thresholds_check(&m, &thresholds, results, KPI_THRESHOLD_RESULT_CAPACITY,
                         &count) != APP_STATUS_OK) {
        puts("FAIL: warning boundary check"); return 1;
    }
    for (index = 0U; index < count; ++index) {
        if (results[index].severity != KPI_SEVERITY_WARNING) {
            puts("FAIL: warning boundary severity"); return 1;
        }
    }

    m.latency_ms = 200.0; m.throughput_mbps = 50.0;
    m.packet_loss_percent = 3.0; m.cpu_utilization_percent = 95.0;
    m.memory_utilization_percent = 95.0;
    if (thresholds_check(&m, &thresholds, results, KPI_THRESHOLD_RESULT_CAPACITY,
                         &count) != APP_STATUS_OK) {
        puts("FAIL: critical boundary check"); return 1;
    }
    for (index = 0U; index < count; ++index) {
        if (results[index].severity != KPI_SEVERITY_CRITICAL) {
            puts("FAIL: critical boundary severity"); return 1;
        }
    }
    stream = tmpfile();
    if (stream == NULL || alerts_generate(stream, &m, results, count) != APP_STATUS_OK) {
        if (stream != NULL) fclose(stream);
        puts("FAIL: alert generation"); return 1;
    }
    rewind(stream);
    bytes = fread(output, 1U, sizeof(output) - 1U, stream);
    output[bytes] = '\0';
    fclose(stream);
    if (strstr(output, "CRITICAL") == NULL || strstr(output, "Latency") == NULL) {
        puts("FAIL: readable alert content"); return 1;
    }
    puts("PASS: normal, warning, critical boundaries, and alert output");
    return 0;
}
