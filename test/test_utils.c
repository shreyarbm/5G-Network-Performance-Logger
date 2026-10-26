#include <stdio.h>
#include <string.h>

#include "utils.h"

static AppStatus read_text(const char *text, char *buffer, size_t capacity)
{
    FILE *input = tmpfile();
    AppStatus status;

    if (input == NULL) return APP_STATUS_ERROR;
    if (fputs(text, input) < 0 || fflush(input) != 0 || fseek(input, 0L, SEEK_SET) != 0) {
        (void)fclose(input);
        return APP_STATUS_ERROR;
    }
    status = utils_read_line_from(input, NULL, NULL, buffer, capacity);
    if (fclose(input) != 0) return APP_STATUS_ERROR;
    return status;
}

int main(void)
{
    char buffer[8];
    char timestamp[32];
    char expected_timestamp[32];
    FILE *empty_input;
    time_t fixed_time = (time_t)1234;
    const struct tm *time_info;

    if (read_text("hello\n", buffer, sizeof(buffer)) != APP_STATUS_OK ||
        strcmp(buffer, "hello") != 0) {
        puts("FAIL: ordinary line input");
        return 1;
    }
    if (read_text("\n", buffer, sizeof(buffer)) != APP_STATUS_OK ||
        buffer[0] != '\0') {
        puts("FAIL: empty line input");
        return 1;
    }
    if (read_text("this line is too long\n", buffer, sizeof(buffer)) != APP_STATUS_INVALID_INPUT ||
        buffer[0] != '\0') {
        puts("FAIL: overlong line handling");
        return 1;
    }
    if (utils_read_line_from(NULL, NULL, NULL, buffer, sizeof(buffer)) != APP_STATUS_INVALID_INPUT) {
        puts("FAIL: invalid arguments");
        return 1;
    }
    if (utils_read_line_from(stdin, NULL, "prompt", buffer, sizeof(buffer)) != APP_STATUS_INVALID_INPUT) {
        puts("FAIL: missing prompt output stream");
        return 1;
    }
    if (utils_read_line_from(stdin, NULL, NULL, buffer, 1U) != APP_STATUS_INVALID_INPUT) {
        puts("FAIL: insufficient buffer capacity");
        return 1;
    }
    empty_input = tmpfile();
    if (empty_input == NULL) return puts("FAIL: create EOF stream"), 1;
    if (utils_read_line_from(empty_input, NULL, NULL, buffer, sizeof(buffer)) != APP_STATUS_ERROR ||
        buffer[0] != '\0') {
        (void)fclose(empty_input);
        puts("FAIL: end-of-file handling");
        return 1;
    }
    if (fclose(empty_input) != 0) return puts("FAIL: close EOF stream"), 1;
    time_info = localtime(&fixed_time);
    if (time_info == NULL ||
        strftime(expected_timestamp, sizeof(expected_timestamp),
                 "%Y-%m-%d %H:%M:%S", time_info) == 0U ||
        utils_format_timestamp(fixed_time, timestamp, sizeof(timestamp)) != APP_STATUS_OK ||
        strcmp(timestamp, expected_timestamp) != 0) {
        puts("FAIL: timestamp format");
        return 1;
    }
    if (utils_format_timestamp((time_t)-1, timestamp, sizeof(timestamp)) != APP_STATUS_INVALID_INPUT ||
        utils_format_timestamp(fixed_time, timestamp, 1U) != APP_STATUS_ERROR) {
        puts("FAIL: invalid timestamp formatting arguments");
        return 1;
    }
    puts("PASS: input handling and timestamp formatting");
    return 0;
}
