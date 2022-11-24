#pragma once

#include <SDL2/SDL.h>

namespace gfx {
    bool in_bounds(SDL_Surface *surface, int x, int y);
    void set_pixel(SDL_Surface *surface, int x, int y, SDL_Color color);
}