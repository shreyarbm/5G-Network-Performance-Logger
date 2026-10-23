#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include "common.h"


AppStatus utils_read_line(const char *prompt, char *buffer, size_t capacity);

#endif

