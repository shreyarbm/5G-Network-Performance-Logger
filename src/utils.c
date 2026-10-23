#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

AppStatus utils_read_line(const char *prompt, char *buffer, size_t capacity)
{
    size_t length;
    int character;

    if (buffer == NULL || capacity < 2U || capacity > (size_t)INT_MAX) {
        return APP_STATUS_INVALID_INPUT;
    }

    if (prompt != NULL) {
        (void)fputs(prompt, stdout);
        (void)fflush(stdout);
    }

    if (fgets(buffer, (int)capacity, stdin) == NULL) {
        buffer[0] = '\0';
        return APP_STATUS_ERROR;
    }

    length = strlen(buffer);
    if (length > 0U && buffer[length - 1U] == '\n') {
        buffer[length - 1U] = '\0';
        return APP_STATUS_OK;
    }

    
    character = getchar();
    if (character != '\n' && character != EOF) {
        while (character != '\n' && character != EOF) {
            character = getchar();
        }
        buffer[0] = '\0';
        return APP_STATUS_INVALID_INPUT;
    }

    return APP_STATUS_OK;
}

