/* lv_stdarg_stub.h — freestanding stub for stdarg.h
 * BUBO OS bare-metal build — no libc available.
 * Uses GCC built-in va_list support which is always available.
 * NO MAS DISADVANTAGED
 */
#ifndef LV_STDARG_STUB_H
#define LV_STDARG_STUB_H

typedef __builtin_va_list va_list;
#define va_start(v, l)  __builtin_va_start(v, l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v, T)    __builtin_va_arg(v, T)
#define va_copy(d, s)   __builtin_va_copy(d, s)

#endif /* LV_STDARG_STUB_H */
