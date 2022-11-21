#include <SDL2/SDL.h>

#include <sstream>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "format.h"
#include "bus.h"
#include "r6502.h"

namespace {
    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;
}

void init_cpu(R6502 &cpu, Bus &bus) {
    std::stringstream program;
    program << "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA";

    int addr = 0;
    while (!program.eof()) {
        std::string byte;
        program >> byte;
        uint8 digit = std::stoi(byte, nullptr, 16);
        bus.rom[addr++] = digit;
    }

    // program starts at ROM_START
    bus.write(0xFFFC, ROM_START & 0xFF);
    bus.write(0xFFFD, ROM_START >> 8);

    cpu.reset(bus);
}

class NesFrontend {
public:
    SDL_Window *window;
    SDL_Surface *window_surface;
//    TTF_Font *font;
    SDL_Surface *screen;

    Bus bus;
    R6502 cpu;
    std::map<uint16, std::string> disassembly;

    NesFrontend() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            panic("could not init SDL");
        }

        window = SDL_CreateWindow("NES",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  NES_WIDTH, NES_HEIGHT,
                                  0);
        if (!window) {
            panic("could not create window");
        }

        window_surface = SDL_GetWindowSurface(window);
        ASSERT(window_surface, "expected window surface");

        screen = SDL_CreateRGBSurface(0, NES_WIDTH, NES_HEIGHT, 24, 0, 0, 0, 0);
        ASSERT(screen, "expected surface");

        SDL_LockSurface(screen);
        for (int i = 0; i < screen->pitch * screen->h; i++) {
            reinterpret_cast<char *>(screen->pixels)[i] = rand() & 0xFF;
        }
        SDL_UnlockSurface(screen);

        init_cpu(cpu, bus);
        disassembly = R6502::disassemble(bus, 0x4020, 0x4040);

        printf("Frontend initialized successfully.\n");
    }

    ~NesFrontend() {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    bool update() {
        SDL_FillRect(window_surface, nullptr, 0);

        render_cpu();

        SDL_BlitSurface(screen, nullptr, window_surface, nullptr);

        SDL_UpdateWindowSurface(window);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                return true;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    return true;
                } else if (event.key.keysym.sym == SDLK_n) {
                    cpu.clock(bus);
                }
                break;
            }
        }

        return false;
    }

    void render_cpu() {
        SDL_LockSurface(screen);
        for (int i = 0; i < screen->pitch * screen->h; i++) {
            reinterpret_cast<char *>(screen->pixels)[i] = rand() & 0xFF;
        }
        SDL_UnlockSurface(screen);
    }
};

#ifdef __EMSCRIPTEN__
void loop_callback(void *userdata) {
    auto frontend = reinterpret_cast<NesFrontend *>(userdata);
    frontend->update();
}

int main() {
    auto frontend = new NesFrontend;
    emscripten_set_main_loop_arg(loop_callback, frontend, 0, 0);
    // we can't destruct NesFrontend in the browser since it will never send an SDL_QUIT event
    return 0;
}
#else
int main() {
    NesFrontend frontend;

    for (;;) {
        if (frontend.update())
            break;
    }

    return 0;
}
#endif
