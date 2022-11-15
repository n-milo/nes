#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "format.h"
#include "bus.h"
#include "r6502.h"

namespace {
    constexpr int VIEWPORT_WIDTH = 720;
    constexpr int VIEWPORT_HEIGHT = 540;

    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;

    constexpr int PADDING = 5;
    constexpr int TEXT_START = 2*PADDING + NES_WIDTH;
}

TTF_Font *font;
SDL_Color white = {255, 255, 255};

void render_text(SDL_Surface *window_surface, int x, int y, const std::string &str) {
    SDL_Surface *message = TTF_RenderText_Solid(font, str.c_str(), white);
    SDL_Rect dst = {x, y, message->w, message->h};
    SDL_BlitSurface(message, nullptr, window_surface, &dst);;
}

void render_cpu(const R6502 &cpu, SDL_Surface *window_surface) {
    auto status = status_to_string(cpu.status);
    render_text(window_surface, TEXT_START, 5,
                string_printf("Status = %02x = %s", cpu.status, status.c_str()));

    render_text(window_surface, TEXT_START, 25,
                string_printf("A = %02x, X = %02x, Y = %02x", cpu.a, cpu.x, cpu.y));

    render_text(window_surface, TEXT_START, 45,
                string_printf("SP = %02x, PC = %04x", cpu.sp, cpu.pc));
}

int main() {
    // TODO: in this file: error handling

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("could not init SDL");
    }

    auto window = SDL_CreateWindow("NES",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
                                   0);
    if (!window) {
        panic("could not create window");
    }

    auto window_surface = SDL_GetWindowSurface(window);
    ASSERT(window_surface, "expected window surface");

    TTF_Init();

    font = TTF_OpenFont("pixel.ttf", 18);
    ASSERT(font, "expected font");

    SDL_Surface *message = TTF_RenderText_Solid(font, "Hello World", white);

    SDL_Surface *screen = SDL_CreateRGBSurface(0, NES_WIDTH, NES_HEIGHT, 24, 0, 0, 0, 0);
    ASSERT(screen, "expected surface");

    SDL_LockSurface(screen);
    for (int i = 0; i < screen->pitch * screen->h; i++) {
        reinterpret_cast<char *>(screen->pixels)[i] = rand() & 0xFF;
    }
    SDL_UnlockSurface(screen);

    Bus bus;
    R6502 cpu;
    cpu.reset(bus);

    bool running = true;
    while (running) {
        render_cpu(cpu, window_surface);

        SDL_Rect dst = {PADDING, PADDING, screen->w, screen->h};
        SDL_BlitSurface(screen, nullptr, window_surface, &dst);

        SDL_UpdateWindowSurface(window);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                break;
            }
        }
    }

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
