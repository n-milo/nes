#pragma once

#include <cstdint>
#include <cstdlib>
#include <utility>
#include "log.h"

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

#ifdef __EMSCRIPTEN__
#define TRAP() abort()
#else
#define TRAP() __builtin_trap()
#endif

#define TODO(...) do { \
    fprintf(stderr, "[%s:%d] TODO: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    TRAP();            \
} while(0)

#define UNREACHABLE(...) do { \
    fprintf(stderr, "[%s:%d] unreachable code reached: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    TRAP();            \
} while(0)

#define panic(...) do { \
    fprintf(stderr, "[%s:%d] PANIC: ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    TRAP();             \
} while(0)

#define ASSERT(cond, ...) do { \
    if (!(cond)) {             \
        fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
        fprintf(stderr, "assertion `%s` failed: ", #cond); \
        fprintf(stderr, __VA_ARGS__);                    \
        fprintf(stderr, "\n"); \
        TRAP();                \
    }                          \
} while(0)

template<typename F>
struct DeferredFunction {
    F f;

    DeferredFunction() = delete;
    DeferredFunction(const DeferredFunction &) = delete;

    DeferredFunction(F &&f) : f(std::move(f)) {}

    ~DeferredFunction() {
        f();
    }
};

struct DeferHelper {

    template<typename F>
    DeferredFunction<F> operator<<(F &&f) {
        return DeferredFunction<F>(std::move(f));
    }

};

inline DeferHelper deferHelper;

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer auto DEFER_3(_defer_) = deferHelper << [&]
