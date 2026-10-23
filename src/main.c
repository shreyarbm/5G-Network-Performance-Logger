#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "alerts.h"
#include "analysis.h"
#include "auth.h"
#include "common.h"
#include "kpi.h"
#include "logger.h"
#include "report.h"
#include "thresholds.h"
#include "utils.h"

#define HISTORY_CAPACITY 1000U
#define INPUT_CAPACITY 64U

static void print_measurement(const KpiMeasurement *measurement)
{
    char timestamp_text[32];
    struct tm *time_info = localtime(&measurement->timestamp);

    if (time_info != NULL && strftime(timestamp_text, sizeof(timestamp_text),
                                      "%Y-%m-%d %H:%M:%S", time_info) != 0U) {
        printf("Timestamp: %s\n", timestamp_text);
    } else {
        puts("Timestamp: unavailable");
    }
    printf("Latency: %.2f ms | Throughput: %.2f Mbps | Packet loss: %.2f%%\n",
           measurement->latency_ms, measurement->throughput_mbps,
           measurement->packet_loss_percent);
    printf("CPU: %.2f%% | Memory: %.2f%% | Source: SIMULATED\n",
           measurement->cpu_utilization_percent,
           measurement->memory_utilization_percent);
}

static int read_choice(int *choice)
{
    char text[INPUT_CAPACITY];
    char *end;
    long value;
    AppStatus input_status;

    input_status = utils_read_line("Select an option: ", text, sizeof(text));
    if (input_status == APP_STATUS_ERROR) return -1;
    if (input_status != APP_STATUS_OK || text[0] == '\0') return 0;
    errno = 0;
    value = strtol(text, &end, 10);
    while (isspace((unsigned char)*end) != 0) end++;
    if (errno != 0 || end == text || *end != '\0' || value < 1L || value > 8L) return 0;
    *choice = (int)value;
    return 1;
}

static AppStatus load_history(KpiMeasurement *records, size_t *count,
                              size_t *malformed)
{
    return logger_load(LOGGER_DEFAULT_PATH, records, HISTORY_CAPACITY,
                       count, malformed);
}

static void print_threshold_results(const KpiThresholdResult *results, size_t count)
{
    size_t index;
    int violations = 0;

    for (index = 0U; index < count; ++index) {
        printf("%s: %s", thresholds_metric_name(results[index].metric),
               thresholds_severity_name(results[index].severity));
        if (results[index].severity != KPI_SEVERITY_NORMAL) {
            printf(" (value %.2f %s; limit %.2f %s)",
                   results[index].measured_value, results[index].unit,
                   results[index].threshold_value, results[index].unit);
            violations = 1;
        }
        putchar('\n');
    }
    printf("Overall: %s\n", violations != 0 ? "VIOLATION" : "NORMAL");
}

static void show_menu(void)
{
    puts("\n--- Main menu ---");
    puts("1. Collect simulated KPI measurement");
    puts("2. View latest measurement");
    puts("3. View historical KPI records");
    puts("4. Analyze historical measurements");
    puts("5. Check latest measurement thresholds");
    puts("6. Generate alerts for latest measurement");
    puts("7. Generate terminal and text-file report");
    puts("8. Exit");
}

int main(void)
{
    AppStatus status;
    KpiMeasurement records[HISTORY_CAPACITY];
    KpiMeasurement current;
    KpiThresholds thresholds;
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY];
    size_t count;
    size_t malformed;
    size_t result_count;
    size_t index;
    int choice;
    int input_result;
    int has_current;

    printf("%s\nVersion: %s\n", APP_TITLE, APP_VERSION);
    status = auth_login();
    if (status != APP_STATUS_OK) {
        puts("Access denied.");
        return status;
    }

    (void)srand((unsigned int)time(NULL));
    thresholds_load_defaults(&thresholds);
    has_current = 0;
    status = load_history(records, &count, &malformed);
    if (status == APP_STATUS_OK && count > 0U) {
        current = records[count - 1U];
        has_current = 1;
    }

    for (;;) {
        show_menu();
        input_result = read_choice(&choice);
        if (input_result == -1) {
            puts("Input closed; exiting.");
            break;
        }
        if (input_result == 0) {
            puts("Invalid choice. Enter a number from 1 to 8.");
            continue;
        }

        if (choice == 1) {
            status = kpi_collect_simulated(&current);
            if (status == APP_STATUS_OK) {
                status = logger_append(LOGGER_DEFAULT_PATH, &current);
            }
            if (status != APP_STATUS_OK) {
                puts("Collection or CSV save failed; check data/ permissions and disk space.");
                continue;
            }
            has_current = 1;
            puts("Saved simulated measurement:");
            print_measurement(&current);
        } else if (choice == 2) {
            if (has_current == 0) {
                puts("No measurement is available yet. Collect one or load a CSV record.");
            } else {
                print_measurement(&current);
            }
        } else if (choice == 3) {
            status = load_history(records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read data/kpi_log.csv (missing, inaccessible, or over capacity).");
                continue;
            }
            printf("Valid records: %lu; malformed rows skipped: %lu\n",
                   (unsigned long)count, (unsigned long)malformed);
            for (index = 0U; index < count; ++index) {
                printf("\nRecord %lu\n", (unsigned long)(index + 1U));
                print_measurement(&records[index]);
            }
            if (count == 0U) puts("No historical measurements found.");
        } else if (choice == 4) {
            status = load_history(records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read historical measurements.");
                continue;
            }
            status = report_write(stdout, records, count, malformed, &thresholds);
            if (status != APP_STATUS_OK) puts("Analysis display failed.");
        } else if (choice == 5 || choice == 6) {
            if (has_current == 0) {
                puts("No measurement is available yet.");
                continue;
            }
            status = thresholds_check(&current, &thresholds, results,
                                      KPI_THRESHOLD_RESULT_CAPACITY, &result_count);
            if (status != APP_STATUS_OK) {
                puts("Threshold checking failed.");
                continue;
            }
            if (choice == 5) {
                puts("Threshold results (demonstration defaults):");
                print_threshold_results(results, result_count);
            } else if (alerts_generate(stdout, &current, results, result_count) != APP_STATUS_OK) {
                puts("Alert output failed.");
            }
        } else if (choice == 7) {
            status = load_history(records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read historical measurements for report.");
                continue;
            }
            status = report_write(stdout, records, count, malformed, &thresholds);
            if (status == APP_STATUS_OK) {
                status = report_generate(REPORT_DEFAULT_PATH, records, count,
                                         malformed, &thresholds);
            }
            if (status == APP_STATUS_OK) {
                printf("Report saved to %s\n", REPORT_DEFAULT_PATH);
            } else {
                puts("Could not generate the report file.");
            }
        } else {
            puts("Goodbye.");
            break;
        }
    }
    return APP_STATUS_OK;
}






