#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include "common.h"


AppStatus utils_read_line(const char *prompt, char *buffer, size_t capacity);
AppStatus utils_read_line_from(FILE *input, FILE *output, const char *prompt,
                              char *buffer, size_t capacity);
AppStatus utils_format_timestamp(time_t timestamp, char *buffer, size_t capacity);

#endif

