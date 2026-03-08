// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// =============================================================================

#ifndef RAVEN_STDINT_H
#define RAVEN_STDINT_H

// ── Exact-width integer types ─────────────────────────────────────────────────
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

// ── Pointer-sized types (64-bit bare metal) ───────────────────────────────────
typedef long long          intptr_t;
typedef unsigned long long uintptr_t;
typedef long long          ptrdiff_t;

// ── Max-width types ───────────────────────────────────────────────────────────
typedef long long          intmax_t;
typedef unsigned long long uintmax_t;

// ── Least-width types (required by LVGL and C99) ─────────────────────────────
typedef int8_t    int_least8_t;
typedef uint8_t   uint_least8_t;
typedef int16_t   int_least16_t;
typedef uint16_t  uint_least16_t;
typedef int32_t   int_least32_t;
typedef uint32_t  uint_least32_t;
typedef int64_t   int_least64_t;
typedef uint64_t  uint_least64_t;

// ── Fast types (required by LVGL internals) ───────────────────────────────────
// On x86-64 bare metal, 32-bit is the fastest general-purpose width.
typedef int32_t   int_fast8_t;
typedef uint32_t  uint_fast8_t;
typedef int32_t   int_fast16_t;
typedef uint32_t  uint_fast16_t;
typedef int32_t   int_fast32_t;
typedef uint32_t  uint_fast32_t;
typedef int64_t   int_fast64_t;
typedef uint64_t  uint_fast64_t;

// ── Limits ────────────────────────────────────────────────────────────────────
#define INT8_MIN     (-128)
#define INT8_MAX     (127)
#define UINT8_MAX    (255U)
#define INT16_MIN    (-32768)
#define INT16_MAX    (32767)
#define UINT16_MAX   (65535U)
#define INT32_MIN    (-2147483648)
#define INT32_MAX    (2147483647)
#define UINT32_MAX   (4294967295U)
#define INT64_MIN    (-9223372036854775807LL - 1)
#define INT64_MAX    (9223372036854775807LL)
#define UINT64_MAX   (18446744073709551615ULL)
#define INTPTR_MIN   INT64_MIN
#define INTPTR_MAX   INT64_MAX
#define UINTPTR_MAX  UINT64_MAX
#define INTMAX_MIN   INT64_MIN
#define INTMAX_MAX   INT64_MAX
#define UINTMAX_MAX  UINT64_MAX
#define SIZE_MAX     UINT64_MAX

// ── Format macros (subset used by LVGL) ──────────────────────────────────────
#define PRId32  "d"
#define PRIu32  "u"
#define PRIx32  "x"
#define PRIX32  "X"
#define PRId64  "lld"
#define PRIu64  "llu"
#define PRIx64  "llx"

#endif /* RAVEN_STDINT_H */
