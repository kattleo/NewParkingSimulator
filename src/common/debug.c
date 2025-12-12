#include "debug.h"
#include <stdio.h>
#include <stdarg.h>

static int debug_enabled = 0;

void debug_set_enabled(int enabled) {
    debug_enabled = enabled;
}

void debug_log(const char *fmt, ...) {
    if (!debug_enabled) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}
