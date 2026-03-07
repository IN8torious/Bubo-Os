#ifndef RAVEN_STDDEF_H
#define RAVEN_STDDEF_H

// Raven OS — Freestanding stddef.h

typedef unsigned int size_t;
typedef int          ssize_t;

#define NULL ((void*)0)

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif // RAVEN_STDDEF_H
