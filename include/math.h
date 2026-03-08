/* math.h — BUBO OS freestanding stub
 * Bare-metal build: -nostdinc blocks the system math.h.
 * Uses GCC built-in math functions (always available, no libm needed).
 * NO MAS DISADVANTAGED
 */
#ifndef BUBO_MATH_H
#define BUBO_MATH_H

static inline float  sqrtf(float x)          { return __builtin_sqrtf(x); }
static inline double sqrt(double x)           { return __builtin_sqrt(x); }
static inline float  fabsf(float x)           { return __builtin_fabsf(x); }
static inline double fabs(double x)           { return __builtin_fabs(x); }
static inline float  floorf(float x)          { return __builtin_floorf(x); }
static inline double floor(double x)          { return __builtin_floor(x); }
static inline float  ceilf(float x)           { return __builtin_ceilf(x); }
static inline double ceil(double x)           { return __builtin_ceil(x); }
static inline float  sinf(float x)            { return __builtin_sinf(x); }
static inline double sin(double x)            { return __builtin_sin(x); }
static inline float  cosf(float x)            { return __builtin_cosf(x); }
static inline double cos(double x)            { return __builtin_cos(x); }
static inline float  tanf(float x)            { return __builtin_tanf(x); }
static inline float  atan2f(float y, float x) { return __builtin_atan2f(y, x); }
static inline double atan2(double y, double x){ return __builtin_atan2(y, x); }
static inline float  powf(float b, float e)   { return __builtin_powf(b, e); }
static inline double pow(double b, double e)  { return __builtin_pow(b, e); }
static inline float  fminf(float a, float b)  { return a < b ? a : b; }
static inline float  fmaxf(float a, float b)  { return a > b ? a : b; }
static inline double fmin(double a, double b) { return a < b ? a : b; }
static inline double fmax(double a, double b) { return a > b ? a : b; }

#define M_PI    3.14159265358979323846
#define M_PI_2  1.57079632679489661923
#define M_E     2.71828182845904523536

#endif /* BUBO_MATH_H */
