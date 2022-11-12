#include <SDL2/SDL.h>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("could not init SDL");
    }
    defer { SDL_Quit(); };

    auto window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, 0);
    if (!window) {
        panic("could not create window");
    }
    defer { SDL_DestroyWindow(window); };

    auto renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        panic("could not create renderer");
    }
    defer { SDL_DestroyRenderer(renderer); };

    for (;;) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                return 0;
            }
        }
    }

    return 0;
}
