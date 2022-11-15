#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace {
    constexpr int VIEWPORT_WIDTH = 720;
    constexpr int VIEWPORT_HEIGHT = 540;

    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;

    constexpr int PADDING = 5;
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

    auto screen_surface = SDL_GetWindowSurface(window);
    ASSERT(screen_surface, "expected screen surface");

    TTF_Init();

    TTF_Font *font = TTF_OpenFont("pixel.ttf", 18);
    ASSERT(font, "expected font");

    SDL_Color white = {255, 255, 255};
    SDL_Surface *message = TTF_RenderText_Solid(font, "Hello World", white);

    SDL_Surface *screen = SDL_CreateRGBSurface(0, NES_WIDTH, NES_HEIGHT, 24, 0, 0, 0, 0);
    ASSERT(screen, "expected surface");

    SDL_LockSurface(screen);
    for (int i = 0; i < screen->pitch * screen->h; i++) {
        reinterpret_cast<char *>(screen->pixels)[i] = rand() & 0xFF;
    }
    SDL_UnlockSurface(screen);

    bool running = true;
    while (running) {
        SDL_Rect dst = {2*PADDING + NES_WIDTH, PADDING, message->w, message->h};
        SDL_BlitSurface(message, nullptr, screen_surface, &dst);;

        dst = {PADDING, PADDING, screen->w, screen->h};
        SDL_BlitSurface(screen, nullptr, screen_surface, &dst);

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
