#pragma once

#define LEVEL_TRACE 4
#define LEVEL_INFO  3
#define LEVEL_WARN  2
#define LEVEL_ERROR 1
#define LEVEL_NONE  0

#ifndef LOG_LEVEL
#define LOG_LEVEL LEVEL_WARN
#endif

#define LOG(label, ...) do { \
    fprintf(stderr, "[%s:%d]" label ": ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
} while(0)

#if LOG_LEVEL >= LEVEL_TRACE
#define LOG_TRACE(...) LOG("[TRACE]", __VA_ARGS__)
#else
#define LOG_TRACE(...)
#endif

#if LOG_LEVEL >= LEVEL_INFO
#define LOG_INFO(...) LOG(" [INFO]", __VA_ARGS__)
#else
#define LOG_INFO(...)
#endif

#if LOG_LEVEL >= LEVEL_WARN
#define LOG_WARN(...) LOG(" [WARN]", __VA_ARGS__)
#else
#define LOG_WARN(...)
#endif

#if LOG_LEVEL >= LEVEL_ERROR
#define LOG_ERROR(...) LOG("[ERROR]", __VA_ARGS__)
#else
#define LOG_ERROR(...)
#endif

