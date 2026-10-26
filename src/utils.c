#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "utils.h"

AppStatus utils_read_line_from(FILE *input, FILE *output, const char *prompt,
                               char *buffer, size_t capacity)
{
    size_t length;
    int character;

    if (input == NULL || buffer == NULL || capacity < 2U ||
        capacity > (size_t)INT_MAX || (prompt != NULL && output == NULL)) {
        return APP_STATUS_INVALID_INPUT;
    }

    if (prompt != NULL) {
        if (fputs(prompt, output) < 0 || fflush(output) != 0) {
            return APP_STATUS_ERROR;
        }
    }

    if (fgets(buffer, (int)capacity, input) == NULL) {
        buffer[0] = '\0';
        return APP_STATUS_ERROR;
    }

    length = strlen(buffer);
    if (length > 0U && buffer[length - 1U] == '\n') {
        buffer[length - 1U] = '\0';
        return APP_STATUS_OK;
    }

    
    character = fgetc(input);
    if (character != '\n' && character != EOF) {
        while (character != '\n' && character != EOF) {
            character = fgetc(input);
        }
        buffer[0] = '\0';
        return APP_STATUS_INVALID_INPUT;
    }

    return APP_STATUS_OK;
}

AppStatus utils_read_line(const char *prompt, char *buffer, size_t capacity)
{
    return utils_read_line_from(stdin, stdout, prompt, buffer, capacity);
}

AppStatus utils_format_timestamp(time_t timestamp, char *buffer, size_t capacity)
{
    const struct tm *time_info;

    if (timestamp == (time_t)-1 || buffer == NULL || capacity == 0U) {
        return APP_STATUS_INVALID_INPUT;
    }

    time_info = localtime(&timestamp);
    if (time_info == NULL ||
        strftime(buffer, capacity, "%Y-%m-%d %H:%M:%S", time_info) == 0U) {
        return APP_STATUS_ERROR;
    }
    return APP_STATUS_OK;
}

