#include "gfx.h"

bool gfx::in_bounds(SDL_Surface *surface, int x, int y) {
    return x >= 0 && y >= 0 && x < surface->w && y < surface->h;
}

void gfx::set_pixel(SDL_Surface *surface, int x, int y, SDL_Color color) {
    ASSERT(surface->locked, "surface should be locked");
    ASSERT(in_bounds(surface, x, y), "pixel out of bounds");

    auto pixels = reinterpret_cast<uint8 *>(surface->pixels);
    auto pixel = reinterpret_cast<uint32 *>(pixels + y*surface->pitch + x*surface->format->BytesPerPixel);
    *pixel = (color.r << 16) | (color.g << 8) | color.b;
}
