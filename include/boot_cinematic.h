// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#ifndef BOOT_CINEMATIC_H
#define BOOT_CINEMATIC_H

#include <stdbool.h>

void boot_cinematic_init(void);
void boot_cinematic_tick(float dt);
bool boot_cinematic_done(void);
void boot_cinematic_skip(void);

#endif
