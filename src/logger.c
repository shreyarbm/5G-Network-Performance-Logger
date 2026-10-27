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

void logger_history_init(KpiHistory *history)
{
    if (history != NULL) {
        history->next_write_index = 0U;
        history->count = 0U;
    }
}

AppStatus logger_history_add(KpiHistory *history,
                             const KpiMeasurement *measurement)
{
    if (history == NULL || measurement == NULL ||
        kpi_validate(measurement) != APP_STATUS_OK ||
        history->next_write_index >= LOGGER_HISTORY_CAPACITY ||
        history->count > LOGGER_HISTORY_CAPACITY) {
        return APP_STATUS_INVALID_INPUT;
    }

    history->records[history->next_write_index] = *measurement;
    history->next_write_index =
        (history->next_write_index + 1U) % LOGGER_HISTORY_CAPACITY;
    if (history->count < LOGGER_HISTORY_CAPACITY) {
        history->count++;
    }
    return APP_STATUS_OK;
}

const KpiMeasurement *logger_history_get(const KpiHistory *history,
                                         size_t chronological_index)
{
    size_t oldest_index;
    size_t physical_index;

    if (history == NULL || history->count > LOGGER_HISTORY_CAPACITY ||
        history->next_write_index >= LOGGER_HISTORY_CAPACITY ||
        chronological_index >= history->count) {
        return NULL;
    }

    oldest_index = (history->count == LOGGER_HISTORY_CAPACITY)
                       ? history->next_write_index
                       : 0U;
    physical_index = (oldest_index + chronological_index) %
                     LOGGER_HISTORY_CAPACITY;
    return &history->records[physical_index];
}

AppStatus logger_history_copy(const KpiHistory *history,
                              KpiMeasurement *records, size_t capacity,
                              size_t *record_count)
{
    size_t index;

    if (history == NULL || record_count == NULL ||
        (records == NULL && history->count > 0U) ||
        history->count > LOGGER_HISTORY_CAPACITY ||
        history->next_write_index >= LOGGER_HISTORY_CAPACITY ||
        capacity < history->count) {
        return APP_STATUS_INVALID_INPUT;
    }

    for (index = 0U; index < history->count; ++index) {
        const KpiMeasurement *measurement = logger_history_get(history, index);
        if (measurement == NULL) {
            return APP_STATUS_ERROR;
        }
        records[index] = *measurement;
    }
    *record_count = history->count;
    return APP_STATUS_OK;
}

AppStatus logger_history_load(const char *path, KpiHistory *history,
                              size_t *malformed_count)
{
    FILE *file;
    char line[LOGGER_LINE_CAPACITY];
    size_t malformed;
    int first_line;
    int file_error;
    KpiMeasurement parsed;

    if (path == NULL || history == NULL || malformed_count == NULL) {
        return APP_STATUS_INVALID_INPUT;
    }

    logger_history_init(history);
    *malformed_count = 0U;
    file = fopen(path, "r");
    if (file == NULL) {
        return APP_STATUS_ERROR;
    }

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
            if (logger_history_add(history, &parsed) != APP_STATUS_OK) {
                (void)fclose(file);
                *malformed_count = malformed;
                return APP_STATUS_ERROR;
            }
        } else {
            malformed++;
        }
    }

    file_error = ferror(file);
    if (fclose(file) != 0) {
        file_error = 1;
    }
    *malformed_count = malformed;
    return file_error == 0 ? APP_STATUS_OK : APP_STATUS_ERROR;
}




