#include <stdio.h>

#include "report.h"

AppStatus report_write(FILE *output, const KpiMeasurement *records, size_t count,
                       size_t malformed_count, const KpiThresholds *thresholds)
{
    KpiStatistics statistics;
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY];
    size_t invalid_count;
    size_t record_index;
    size_t result_index;
    size_t violations;
    size_t analyzed_records;
    AppStatus status;

    if (output == NULL || thresholds == NULL || (records == NULL && count > 0U)) {
        return APP_STATUS_INVALID_INPUT;
    }

    status = analysis_calculate(records, count, &statistics, &invalid_count);
    if (status != APP_STATUS_OK) {
        return status;
    }

    violations = 0U;
    analyzed_records = 0U;
    for (record_index = 0U; record_index < count; ++record_index) {
        if (kpi_validate(&records[record_index]) != APP_STATUS_OK) {
            continue;
        }
        status = thresholds_check(&records[record_index], thresholds, results,
                                  KPI_THRESHOLD_RESULT_CAPACITY, &analyzed_records);
        if (status != APP_STATUS_OK) {
            return status;
        }
        for (result_index = 0U; result_index < analyzed_records; ++result_index) {
            if (results[result_index].severity != KPI_SEVERITY_NORMAL) {
                violations++;
            }
        }
    }

    if (fprintf(output, "5G Telecom Network Performance Logger report\n"
                "Data source: SIMULATED ONLY (not actual 5G performance)\n"
                "Records analyzed: %lu\nMalformed CSV rows skipped: %lu\n"
                "Invalid measurements skipped: %lu\n",
                (unsigned long)statistics.record_count,
                (unsigned long)malformed_count,
                (unsigned long)invalid_count) < 0) {
        return APP_STATUS_ERROR;
    }

    if (statistics.record_count == 0U) {
        if (fputs("No valid measurements are available for statistics.\n", output) == EOF) {
            return APP_STATUS_ERROR;
        }
    } else {
#define REPORT_METRIC(label, average, minimum, maximum, unit) \
        if (fprintf(output, label ": avg %.2f " unit ", min %.2f " unit ", max %.2f " unit "\n", \
                    statistics.average, statistics.minimum, statistics.maximum) < 0) return APP_STATUS_ERROR
        REPORT_METRIC("Latency", average_latency_ms, minimum_latency_ms, maximum_latency_ms, "ms");
        REPORT_METRIC("Throughput", average_throughput_mbps, minimum_throughput_mbps, maximum_throughput_mbps, "Mbps");
        REPORT_METRIC("Packet loss", average_packet_loss_percent, minimum_packet_loss_percent, maximum_packet_loss_percent, "%%");
        REPORT_METRIC("CPU utilization", average_cpu_utilization_percent, minimum_cpu_utilization_percent, maximum_cpu_utilization_percent, "%%");
        REPORT_METRIC("Memory utilization", average_memory_utilization_percent, minimum_memory_utilization_percent, maximum_memory_utilization_percent, "%%");
#undef REPORT_METRIC
    }

    if (fprintf(output, "Data quality: %s\nThreshold KPI violations: %lu\nOverall status: %s\n",
                (malformed_count > 0U || invalid_count > 0U) ? "INCOMPLETE" : "COMPLETE",
                (unsigned long)violations,
                violations == 0U ? (statistics.record_count == 0U ? "NO DATA" : "NORMAL") : "VIOLATION") < 0) {
        return APP_STATUS_ERROR;
    }
    return ferror(output) == 0 ? APP_STATUS_OK : APP_STATUS_ERROR;
}

AppStatus report_generate(const char *path, const KpiMeasurement *records,
                          size_t count, size_t malformed_count,
                          const KpiThresholds *thresholds)
{
    FILE *file;
    AppStatus status;
    int close_status;

    if (path == NULL) {
        return APP_STATUS_INVALID_INPUT;
    }

    file = fopen(path, "w");
    if (file == NULL) {
        return APP_STATUS_ERROR;
    }
    status = report_write(file, records, count, malformed_count, thresholds);
    if (fflush(file) != 0) {
        status = APP_STATUS_ERROR;
    }
    close_status = fclose(file);
    if (close_status != 0) {
        status = APP_STATUS_ERROR;
    }
    return status;
}





