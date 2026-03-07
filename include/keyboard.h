#ifndef RAVEN_KEYBOARD_H
#define RAVEN_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void keyboard_init(void);
char keyboard_getchar(void);
bool keyboard_has_input(void);

#endif // RAVEN_KEYBOARD_H
