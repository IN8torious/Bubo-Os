#ifndef RAVEN_SHELL_H
#define RAVEN_SHELL_H

#include "idt.h"

// Initialize the CORVUS shell
void shell_init(void);

// Print the shell prompt
void shell_print_prompt(void);

// Submit a command line (called by keyboard driver on Enter)
void corvus_shell_submit(const char* line);

#endif // RAVEN_SHELL_H
