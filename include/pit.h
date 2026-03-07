#ifndef RAVEN_PIT_H
#define RAVEN_PIT_H

#include <stdint.h>

// Initialize PIT at given frequency (Hz)
void pit_init(uint32_t frequency);

// Sleep for ms milliseconds (busy-wait)
void pit_sleep(uint32_t ms);

// Get current tick count
uint32_t pit_get_ticks(void);

#endif // RAVEN_PIT_H
