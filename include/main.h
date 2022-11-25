#pragma once

#if !defined(LOAD_DYNAMICALLY) && !defined(__EMSCRIPTEN__)
#define LOAD_DYNAMICALLY 1
#endif

#if !LOAD_DYNAMICALLY
extern "C" {
    void *frontend_init();
    bool frontend_update(void *arg); // returns true if game should close
    void frontend_close(void *arg);
}
#else
typedef void *(*frontend_init_t)();
typedef bool (*frontend_update_t)(void *arg);
typedef void (*frontend_close_t)(void *arg);
#endif