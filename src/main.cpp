#include "main.h"

#ifdef __EMSCRIPTEN__

#if LOAD_DYNAMICALLY
#error Cannot load emscripten dynamically
#endif

void loop_callback(void *arg) {
    frontend_update(arg);
}

int main() {
    auto arg = frontend_init();
    emscripten_set_main_loop_arg(loop_callback, arg, 0, 0);
    // we can't destruct NesFrontend in the browser since it will never send an SDL_QUIT event
    return 0;
}
#elif LOAD_DYNAMICALLY

#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <dlfcn.h>

const char *path = "cmake-build-debug/libnes_frontend.dylib";

void *lib;
frontend_init_t frontend_init;
frontend_update_t frontend_update;
frontend_close_t frontend_close;

void reload_symbols() {
    if (lib) {
        dlclose(lib);
    }

    lib = dlopen(path, RTLD_LAZY);
    if (!lib) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    frontend_init = (frontend_init_t) dlsym(lib, "frontend_init");
    if (!frontend_init) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    frontend_update = (frontend_update_t) dlsym(lib, "frontend_update");
    if (!frontend_update) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    frontend_close = (frontend_close_t) dlsym(lib, "frontend_close");
    if (!frontend_close) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
}

void xstat(const char *p, struct stat *buf) {
    int err = stat(p, buf);
    if (err == -1) {
        perror("stat");
        exit(1);
    }
}

int main() {
    reload_symbols();
    auto arg = frontend_init();

    struct stat buf = {};
    xstat(path, &buf);
    auto last_time = buf.st_mtimespec;

    for (;;) {
        xstat(path, &buf);
        auto now = buf.st_mtimespec;
        if (last_time.tv_sec != now.tv_sec || last_time.tv_nsec != now.tv_nsec) {
            LOG_INFO("reloading file");
            reload_symbols();
            last_time = now;
        }

        if (frontend_update(arg))
            break;
    }

    frontend_close(arg);
    return 0;
}


#else
int main() {
    auto arg = frontend_init();

    for (;;) {
        if (frontend_update(arg))
            break;
    }

    frontend_close(arg);
    return 0;
}
#endif
