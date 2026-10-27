#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "alerts.h"
#include "analysis.h"
#include "auth.h"
#include "collector.h"
#include "common.h"
#include "kpi.h"
#include "logger.h"
#include "report.h"
#include "thresholds.h"
#include "utils.h"

#define HISTORY_CAPACITY LOGGER_HISTORY_CAPACITY
#define INPUT_CAPACITY 64U

static void print_measurement(const KpiMeasurement *measurement)
{
    char timestamp_text[32];

    if (utils_format_timestamp(measurement->timestamp, timestamp_text,
                               sizeof(timestamp_text)) == APP_STATUS_OK) {
        printf("Timestamp: %s\n", timestamp_text);
    } else {
        puts("Timestamp: unavailable");
    }
    printf("Latency: %.2f ms | Throughput: %.2f Mbps | Packet loss: %.2f%%\n",
           measurement->latency_ms, measurement->throughput_mbps,
           measurement->packet_loss_percent);
    printf("CPU: %.2f%% | Memory: %.2f%%\n",
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
    if (errno != 0 || end == text || *end != '\0' || value < 1L || value > 9L) return 0;
    *choice = (int)value;
    return 1;
}

static AppStatus load_history(KpiCollector *collector, KpiHistory *history,
                              KpiMeasurement *records,
                              size_t *count, size_t *malformed)
{
    AppStatus status = collector_load_history(collector, history, malformed);
    if (status != APP_STATUS_OK) {
        *count = 0U;
        return status;
    }
    return logger_history_copy(history, records, HISTORY_CAPACITY, count);
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
    puts("8. Start or stop background simulated collection");
    puts("9. Exit");
}

int main(void)
{
    AppStatus status;
    KpiCollector collector;
    KpiHistory history;
    KpiMeasurement records[HISTORY_CAPACITY];
    KpiMeasurement current;
    KpiThresholds thresholds;
    KpiThresholdResult results[KPI_THRESHOLD_RESULT_CAPACITY];
    size_t count;
    size_t malformed;
    size_t result_count;
    size_t index;
    int choice;
    int has_current;

    printf("%s\nVersion: %s\n", APP_TITLE, APP_VERSION);
    status = auth_login();
    if (status != APP_STATUS_OK) {
        puts("Access denied.");
        return status;
    }

    status = collector_init(&collector, LOGGER_DEFAULT_PATH,
                            COLLECTOR_DEFAULT_INTERVAL_SECONDS);
    if (status != APP_STATUS_OK) {
        puts("Could not initialize background collector synchronization.");
        return status;
    }

    (void)srand((unsigned int)time(NULL));
    thresholds_load_defaults(&thresholds);
    has_current = 0;
    status = load_history(&collector, &history, records, &count, &malformed);
    if (status == APP_STATUS_OK && count > 0U) {
        status = collector_set_latest(&collector, &records[count - 1U]);
        if (status != APP_STATUS_OK) {
            (void)collector_destroy(&collector);
            puts("Could not initialize the latest measurement.");
            return status;
        }
    }

    for (;;) {
        if (collector_take_status(&collector) != APP_STATUS_OK) {
            puts("Background collection or CSV save failed.");
        }
        show_menu();
        int input_result = read_choice(&choice);
        if (input_result == -1) {
            puts("Input closed; exiting.");
            break;
        }
        if (input_result == 0) {
            puts("Invalid choice. Enter a number from 1 to 9.");
            continue;
        }

        if (choice == 1) {
            status = collector_collect_now(&collector, &current);
            if (status != APP_STATUS_OK) {
                puts("Collection or CSV save failed; check data/ permissions and disk space.");
                continue;
            }
            has_current = 1;
            puts("Saved simulated measurement:");
            print_measurement(&current);
        } else if (choice == 2) {
            status = collector_get_latest(&collector, &current);
            has_current = status == APP_STATUS_OK;
            if (has_current == 0) {
                puts("No measurement is available yet. Collect one or start background collection.");
            } else {
                print_measurement(&current);
            }
        } else if (choice == 3) {
            status = load_history(&collector, &history, records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read data/kpi_log.csv (missing or inaccessible).");
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
            status = load_history(&collector, &history, records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read historical measurements.");
                continue;
            }
            status = report_write(stdout, records, count, malformed, &thresholds);
            if (status != APP_STATUS_OK) puts("Analysis display failed.");
        } else if (choice == 5 || choice == 6) {
            status = collector_get_latest(&collector, &current);
            has_current = status == APP_STATUS_OK;
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
            char report_path[REPORT_PATH_CAPACITY];
            status = load_history(&collector, &history, records, &count, &malformed);
            if (status != APP_STATUS_OK) {
                puts("Could not read historical measurements for report.");
                continue;
            }
            status = report_write(stdout, records, count, malformed, &thresholds);
            if (status == APP_STATUS_OK) {
                status = report_generate_numbered(records, count, malformed,
                                                  &thresholds, report_path,
                                                  sizeof(report_path));
            }
            if (status == APP_STATUS_OK) {
                printf("Report saved to %s\n", report_path);
            } else {
                puts("Could not generate the report file.");
            }
        } else if (choice == 8) {
            int is_running;
            status = collector_is_running(&collector, &is_running);
            if (status == APP_STATUS_OK) {
                status = (is_running != 0) ? collector_stop(&collector)
                                           : collector_start(&collector);
            }
            if (status == APP_STATUS_OK) {
                puts(is_running != 0 ? "Background collection stopped."
                                     : "Background simulated collection started (5-second interval).");
            } else {
                puts("Could not change background collection state.");
            }
        } else {
            break;
        }
    }
    if (collector_destroy(&collector) != APP_STATUS_OK) {
        puts("Could not cleanly stop the background collector.");
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}

