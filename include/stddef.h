// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// =============================================================================

#ifndef RAVEN_STDDEF_H
#define RAVEN_STDDEF_H

// Raven OS — Freestanding stddef.h (x86-64 bare metal)
// size_t and ssize_t must be 64-bit to match intptr_t / uintptr_t.

typedef unsigned long long  size_t;
typedef long long           ssize_t;
typedef unsigned int        wchar_t;  /* freestanding stub */

#define NULL ((void*)0)

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif /* RAVEN_STDDEF_H */
