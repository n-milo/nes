#include "font.h"

#include <fstream>

#include <nlohmann/json.hpp>

Font::Font(const char *bitmap_json) {
    std::ifstream f(bitmap_json);
    ASSERT(f.is_open(), "could not find '%s'", bitmap_json);
    characters = nlohmann::json::parse(f);
}

SDL_Surface *Font::render_text(std::string_view text, SDL_Color color, int scale) const {
    int width = static_cast<int>(text.length()) * (TEXT_COLS + 1) * scale;
    SDL_Surface *surface = SDL_CreateRGBSurface(0, width, TEXT_ROWS * scale, 32, 0, 0, 0, 0xFF000000);
    ASSERT(surface, "SDL_CreateRGBSurface failed: %s", SDL_GetError());
    render_to_surface(surface, 0, 0, text, color, scale);
    return surface;
}

void Font::render_to_surface(SDL_Surface *surface, int x, int y, std::string_view text, SDL_Color color, int scale) const {
    SDL_LockSurface(surface);

    auto pixels = reinterpret_cast<uint32 *>(surface->pixels);
    int xadvance = 0;
    for (auto &c : text) {
        auto s = std::string(1, c);
        auto it = characters.find(s);
        ASSERT(it != characters.end(), "unexpected character or string '%s'", s.c_str());

        auto &rows = it->second;
        for (int row = 0; row < TEXT_ROWS; row++) {
            for (int col = 0; col < TEXT_COLS; col++) {

                for (int pixel_y = 0; pixel_y < scale; pixel_y++) {
                    for (int pixel_x = 0; pixel_x < scale; pixel_x++) {
                        int bit = rows[row] & (1 << col);
                        int tx = (xadvance + col) * scale + pixel_x + x;
                        int ty = row * scale + pixel_y + y;
                        if (bit)
                            pixels[tx + ty * surface->w] = 0xFF000000 | (color.r << 16) | (color.g << 8) | color.b;
                    }
                }
            }
        }

        xadvance += TEXT_COLS+1;
    }

    SDL_UnlockSurface(surface);
}