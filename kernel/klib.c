/*
 * klib.c — Minimal freestanding C library for BUBO OS kernel
 * Provides: snprintf, vsnprintf, memset, memcpy, sqrtf
 *
 * Built by Nathan Brown. N8torious AI. Blue OS.
 * NO MAS DISADVANTAGED.
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* ── memset ──────────────────────────────────────────────────────────────── */
void *memset(void *s, int c, size_t n)
{
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

/* ── memcpy ──────────────────────────────────────────────────────────────── */
void *memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

/* ── strlen ──────────────────────────────────────────────────────────────── */
size_t strlen(const char *s)
{
    size_t n = 0;
    while (*s++) n++;
    return n;
}

/* ── sqrtf (Newton-Raphson, good enough for particles) ───────────────────── */
float sqrtf(float x)
{
    if (x <= 0.0f) return 0.0f;
    float r = x;
    for (int i = 0; i < 16; i++)
        r = 0.5f * (r + x / r);
    return r;
}

/* ── vsnprintf / snprintf ────────────────────────────────────────────────── */
static void _put_char(char *buf, size_t *pos, size_t size, char c)
{
    if (*pos < size - 1)
        buf[(*pos)++] = c;
}

static void _put_str(char *buf, size_t *pos, size_t size, const char *s)
{
    while (s && *s)
        _put_char(buf, pos, size, *s++);
}

static void _put_uint(char *buf, size_t *pos, size_t size,
                      unsigned long long v, int base, int upper)
{
    const char *digits_lo = "0123456789abcdef";
    const char *digits_up = "0123456789ABCDEF";
    const char *digits = upper ? digits_up : digits_lo;
    char tmp[32];
    int  len = 0;
    if (v == 0) { _put_char(buf, pos, size, '0'); return; }
    while (v) { tmp[len++] = digits[v % (unsigned)base]; v /= (unsigned)base; }
    while (len--) _put_char(buf, pos, size, tmp[len + 1 - 1 + len - len]);
    /* simple reverse */
    /* rewrite cleanly */
    (void)tmp; (void)len;
}

/* Cleaner integer writer */
static void _write_uint(char *buf, size_t *pos, size_t size,
                        unsigned long long v, int base, int upper)
{
    const char *lo = "0123456789abcdef";
    const char *up = "0123456789ABCDEF";
    const char *d  = upper ? up : lo;
    char tmp[32]; int n = 0;
    if (v == 0) { _put_char(buf, pos, size, '0'); return; }
    while (v) { tmp[n++] = d[v % (unsigned)base]; v /= (unsigned)base; }
    for (int i = n - 1; i >= 0; i--)
        _put_char(buf, pos, size, tmp[i]);
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    if (!buf || size == 0) return 0;
    size_t pos = 0;
    while (*fmt) {
        if (*fmt != '%') { _put_char(buf, &pos, size, *fmt++); continue; }
        fmt++;
        switch (*fmt) {
        case 'd': case 'i': {
            long long v = va_arg(ap, int);
            if (v < 0) { _put_char(buf, &pos, size, '-'); v = -v; }
            _write_uint(buf, &pos, size, (unsigned long long)v, 10, 0);
            break;
        }
        case 'u': _write_uint(buf, &pos, size, va_arg(ap, unsigned), 10, 0); break;
        case 'x': _write_uint(buf, &pos, size, va_arg(ap, unsigned), 16, 0); break;
        case 'X': _write_uint(buf, &pos, size, va_arg(ap, unsigned), 16, 1); break;
        case 'p': {
            _put_str(buf, &pos, size, "0x");
            _write_uint(buf, &pos, size,
                (unsigned long long)(uintptr_t)va_arg(ap, void*), 16, 0);
            break;
        }
        case 's': _put_str(buf, &pos, size, va_arg(ap, const char *)); break;
        case 'c': _put_char(buf, &pos, size, (char)va_arg(ap, int)); break;
        case '%': _put_char(buf, &pos, size, '%'); break;
        default:  _put_char(buf, &pos, size, '%');
                  _put_char(buf, &pos, size, *fmt); break;
        }
        fmt++;
    }
    buf[pos] = '\0';
    return (int)pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}
