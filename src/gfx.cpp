#include "gfx.h"

SDL_Surface *gfx::create_surface(int w, int h) {
    SDL_Surface *surface = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
    ASSERT(surface, "expected surface");
    return surface;
}

bool gfx::in_bounds(SDL_Surface *surface, int x, int y) {
    return x >= 0 && y >= 0 && x < surface->w && y < surface->h;
}

void gfx::set_pixel(SDL_Surface *surface, int x, int y, SDL_Color color) {
    ASSERT(surface->locked, "surface should be locked");
    ASSERT(in_bounds(surface, x, y), "pixel out of bounds");

    auto pixels = reinterpret_cast<uint8 *>(surface->pixels);
    auto pixel = reinterpret_cast<uint32 *>(pixels + y*surface->pitch + x*surface->format->BytesPerPixel);
    uint32 rgb = SDL_MapRGB(surface->format, color.r, color.g, color.b);
    *pixel = rgb;
}
