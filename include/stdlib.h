/* stdlib.h — BUBO OS freestanding stub
 * Bare-metal build: -nostdinc blocks the system stdlib.h.
 * Provides the subset used by kernel code.
 * malloc/free are NOT provided — use pmm_alloc() instead.
 * NO MAS DISADVANTAGED
 */
#ifndef BUBO_STDLIB_H
#define BUBO_STDLIB_H

#include <stddef.h>

/* Absolute value */
static inline int abs(int x)       { return x < 0 ? -x : x; }
static inline long labs(long x)    { return x < 0 ? -x : x; }

/* Simple atoi */
static inline int atoi(const char *s) {
    int n = 0, neg = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    else if (*s == '+') s++;
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

/* No-op abort for bare metal — triggers a kernel panic via halt */
static inline void abort(void) {
    __asm__ volatile("cli; hlt");
    __builtin_unreachable();
}

#endif /* BUBO_STDLIB_H */
