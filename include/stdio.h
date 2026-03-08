/* stdio.h — BUBO OS freestanding stub
 * Bare-metal build: -nostdinc blocks the system stdio.h.
 * We provide snprintf/vsnprintf via GCC built-ins.
 * printf is NOT provided — use terminal_write() for output.
 * NO MAS DISADVANTAGED
 */
#ifndef BUBO_STDIO_H
#define BUBO_STDIO_H

#include <stddef.h>
#include "lv_stdarg_stub.h"  /* freestanding va_list — no system stdarg.h available */

/* snprintf / vsnprintf — GCC provides __builtin_vsnprintf on freestanding */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif /* BUBO_STDIO_H */
