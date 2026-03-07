// =============================================================================
// INSTINCT ENGINE — Boot Cinematic Header
// Instinct OS v1.1 | Built by Nathan Samuel (IN8torious)
// "NO MAS DISADVANTAGED"
// =============================================================================

#ifndef BOOT_CINEMATIC_H
#define BOOT_CINEMATIC_H

#include <stdbool.h>

void boot_cinematic_init(void);
void boot_cinematic_tick(float dt);
bool boot_cinematic_done(void);
void boot_cinematic_skip(void);

#endif
