#include <stdio.h>
#include <string.h>

#include "logger.h"

#define LOGGER_LINE_CAPACITY 256U

AppStatus logger_append(const char *path, const KpiMeasurement *measurement)
{
    FILE *file;
    long file_size;
    int write_status;
    int close_status;

    if (path == NULL || measurement == NULL ||
        kpi_validate(measurement) != APP_STATUS_OK) {
        return APP_STATUS_INVALID_INPUT;
    }

    file = fopen(path, "a+");
    if (file == NULL) {
        return APP_STATUS_ERROR;
    }

    if (fseek(file, 0L, SEEK_END) != 0) {
        (void)fclose(file);
        return APP_STATUS_ERROR;
    }
    file_size = ftell(file);
    if (file_size < 0L) {
        (void)fclose(file);
        return APP_STATUS_ERROR;
    }

    if (file_size == 0L &&
        fprintf(file, "%s\n", LOGGER_CSV_HEADER) < 0) {
        (void)fclose(file);
        return APP_STATUS_ERROR;
    }

    write_status = fprintf(file, "%lld,%.2f,%.2f,%.2f,%.2f,%.2f,SIMULATED\n",
                           (long long)measurement->timestamp,
                           measurement->latency_ms,
                           measurement->throughput_mbps,
                           measurement->packet_loss_percent,
                           measurement->cpu_utilization_percent,
                           measurement->memory_utilization_percent);
    if (write_status < 0 || fflush(file) != 0) {
        (void)fclose(file);
        return APP_STATUS_ERROR;
    }

    close_status = fclose(file);
    return (close_status == 0) ? APP_STATUS_OK : APP_STATUS_ERROR;
}

static int logger_parse_row(const char *line, KpiMeasurement *measurement)
{
    long long timestamp_value;
    char source[16];
    char extra;
    int fields;
    size_t comma_count;
    const char *cursor;

    comma_count = 0U;
    for (cursor = line; *cursor != '\0'; ++cursor) {
        if (*cursor == ',') {
            comma_count++;
        }
    }
    if (comma_count != 6U) {
        return 0;
    }

    fields = sscanf(line, "%lld,%lf,%lf,%lf,%lf,%lf,%15[^,],%c",
                    &timestamp_value,
                    &measurement->latency_ms,
                    &measurement->throughput_mbps,
                    &measurement->packet_loss_percent,
                    &measurement->cpu_utilization_percent,
                    &measurement->memory_utilization_percent,
                    source, &extra);
    if (fields != 7 || strcmp(source, "SIMULATED") != 0) {
        return 0;
    }

    measurement->timestamp = (time_t)timestamp_value;
    measurement->source = KPI_SOURCE_SIMULATED;
    if ((long long)measurement->timestamp != timestamp_value ||
        kpi_validate(measurement) != APP_STATUS_OK) {
        return 0;
    }
    return 1;
}

AppStatus logger_load(const char *path, KpiMeasurement *records,
                      size_t capacity, size_t *record_count,
                      size_t *malformed_count)
{
    FILE *file;
    char line[LOGGER_LINE_CAPACITY];
    size_t loaded;
    size_t malformed;
    int first_line;
    int file_error;
    KpiMeasurement parsed;

    if (path == NULL || records == NULL || record_count == NULL ||
        malformed_count == NULL || capacity == 0U) {
        return APP_STATUS_INVALID_INPUT;
    }

    *record_count = 0U;
    *malformed_count = 0U;
    file = fopen(path, "r");
    if (file == NULL) {
        return APP_STATUS_ERROR;
    }

    loaded = 0U;
    malformed = 0U;
    first_line = 1;
    while (fgets(line, (int)sizeof(line), file) != NULL) {
        size_t length = strlen(line);
        int complete = (length > 0U && line[length - 1U] == '\n');

        if (complete != 0) {
            line[length - 1U] = '\0';
        } else if (!feof(file)) {
            int character = fgetc(file);
            while (character != '\n' && character != EOF) {
                character = fgetc(file);
            }
            malformed++;
            first_line = 0;
            continue;
        }

        if (first_line != 0 && strcmp(line, LOGGER_CSV_HEADER) == 0) {
            first_line = 0;
            continue;
        }
        first_line = 0;

        if (logger_parse_row(line, &parsed) != 0) {
            if (loaded >= capacity) {
                (void)fclose(file);
                *record_count = loaded;
                *malformed_count = malformed;
                return APP_STATUS_ERROR;
            }
            records[loaded] = parsed;
            loaded++;
        } else {
            malformed++;
        }
    }

    file_error = ferror(file);
    if (fclose(file) != 0) {
        file_error = 1;
    }
    if (file_error != 0) {
        *record_count = loaded;
        *malformed_count = malformed;
        return APP_STATUS_ERROR;
    }

    *record_count = loaded;
    *malformed_count = malformed;
    return APP_STATUS_OK;
}




