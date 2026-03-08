/* lv_inttypes_stub.h — freestanding stub for inttypes.h
 * BUBO OS bare-metal build — no libc available.
 * Provides only the PRIx macros LVGL actually uses.
 * NO MAS DISADVANTAGED
 */
#ifndef LV_INTTYPES_STUB_H
#define LV_INTTYPES_STUB_H

#include <stdint.h>

#define PRId8   "d"
#define PRId16  "d"
#define PRId32  "d"
#define PRId64  "lld"
#define PRIu8   "u"
#define PRIu16  "u"
#define PRIu32  "u"
#define PRIu64  "llu"
#define PRIx8   "x"
#define PRIx16  "x"
#define PRIx32  "x"
#define PRIx64  "llx"
#define PRIX8   "X"
#define PRIX16  "X"
#define PRIX32  "X"
#define PRIX64  "llX"

#endif /* LV_INTTYPES_STUB_H */
