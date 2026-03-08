// =============================================================================
// BUBO OS — Safety Flask Header
// Copyright (c) 2025 N8torious AI. MIT License.
// =============================================================================
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Seal the Flask — call once at the very start of kernel_main
void safety_flask_init(void);

// Trigger a kernel panic — displays error screen and halts forever
void __attribute__((noreturn)) kernel_panic(const char* msg);

// Exception handlers — wire to IDT
void flask_handle_divide_by_zero(void);
void flask_handle_general_protection(void);
void flask_handle_page_fault(void);
void flask_handle_double_fault(void);
void flask_handle_stack_fault(void);
void flask_handle_invalid_opcode(void);

// Convenience macro — use anywhere in kernel code
#define PANIC(msg) kernel_panic("[" __FILE__ ":" __STRINGIFY(__LINE__) "] " msg)
#define __STRINGIFY(x) __STRINGIFY2(x)
#define __STRINGIFY2(x) #x

#ifdef __cplusplus
}
#endif
