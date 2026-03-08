/* string.h — BUBO OS freestanding stub
 * Bare-metal build: -nostdinc blocks the system string.h.
 * We provide the subset BUBO OS kernel code actually uses,
 * implemented via GCC built-ins (always available, no libc needed).
 * NO MAS DISADVANTAGED
 */
#ifndef BUBO_STRING_H
#define BUBO_STRING_H

#include <stddef.h>

static inline void *memset(void *s, int c, size_t n) {
    return __builtin_memset(s, c, n);
}
static inline void *memcpy(void *dst, const void *src, size_t n) {
    return __builtin_memcpy(dst, src, n);
}
static inline void *memmove(void *dst, const void *src, size_t n) {
    return __builtin_memmove(dst, src, n);
}
static inline int memcmp(const void *a, const void *b, size_t n) {
    return __builtin_memcmp(a, b, n);
}
static inline size_t strlen(const char *s) {
    return __builtin_strlen(s);
}
static inline int strcmp(const char *a, const char *b) {
    return __builtin_strcmp(a, b);
}
static inline int strncmp(const char *a, const char *b, size_t n) {
    return __builtin_strncmp(a, b, n);
}
static inline char *strcpy(char *dst, const char *src) {
    return __builtin_strcpy(dst, src);
}
static inline char *strncpy(char *dst, const char *src, size_t n) {
    return __builtin_strncpy(dst, src, n);
}
static inline char *strcat(char *dst, const char *src) {
    return __builtin_strcat(dst, src);
}
static inline char *strchr(const char *s, int c) {
    return __builtin_strchr(s, c);
}
static inline char *strrchr(const char *s, int c) {
    return __builtin_strrchr(s, c);
}
static inline char *strstr(const char *haystack, const char *needle) {
    return __builtin_strstr(haystack, needle);
}

#endif /* BUBO_STRING_H */
