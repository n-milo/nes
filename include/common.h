#pragma once

#include <cstdint>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#define TODO(...) do { \
    fprintf(stderr, "[%s:%d] TODO: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    __builtin_trap();  \
} while(0)

#define panic(...) do { \
    fprintf(stderr, "[%s:%d] PANIC: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    __builtin_trap();  \
} while(0)

#define ASSERT(cond, ...) do { \
    if (!(cond)) {             \
        fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
        fprintf(stderr, "assertion `%s` failed: ", #cond); \
        fprintf(stderr, __VA_ARGS__);                    \
        __builtin_trap();      \
    }                          \
} while(0)
