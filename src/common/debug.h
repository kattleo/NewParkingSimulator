#ifndef DEBUG_H
#define DEBUG_H

#include <stdarg.h>

// Set debug flag (1=on, 0=off)
void debug_set_enabled(int enabled);
// Print debug log if enabled
void debug_log(const char *fmt, ...);

#endif // DEBUG_H
