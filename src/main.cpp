#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("could not init SDL");
    }

    auto window = SDL_CreateWindow("NES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 240, 0);
    if (!window) {
        panic("could not create window");
    }

    auto renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer) {
        panic("could not create renderer");
    }

    TTF_Init();

    TTF_Font *font = TTF_OpenFont("pixel.ttf", 24);
    ASSERT(font, "expected font");

    SDL_Color white = {255, 255, 255};
    SDL_Surface *message_surface = TTF_RenderText_Solid(font, "Hello World", white);
    SDL_Texture *message = SDL_CreateTextureFromSurface(renderer, message_surface);

    bool running = true;
    while (running) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect message_rect = {0, 0, message_surface->w, message_surface->h};
        SDL_RenderCopy(renderer, message, nullptr, &message_rect);

        SDL_RenderPresent(renderer);

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

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
