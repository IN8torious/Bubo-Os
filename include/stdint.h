// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

#ifndef RAVEN_STDINT_H
#define RAVEN_STDINT_H

// Raven OS — Freestanding stdint.h
// No OS or libc underneath — define our own fixed-width types

typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;

typedef int32_t            ptrdiff_t;

#define INT8_MIN    (-128)
#define INT8_MAX    (127)
#define UINT8_MAX   (255U)
#define INT16_MIN   (-32768)
#define INT16_MAX   (32767)
#define UINT16_MAX  (65535U)
#define INT32_MIN   (-2147483648)
#define INT32_MAX   (2147483647)
#define UINT32_MAX  (4294967295U)

#endif // RAVEN_STDINT_H
