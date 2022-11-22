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

                for (int y = 0; y < scale; y++) {
                    for (int x = 0; x < scale; x++) {
                        int bit = rows[row] & (1 << col);
                        int tx = (xadvance + col) * scale + x;
                        int ty = row * scale + y;
                        if (bit)
                            pixels[tx + ty * width] = (color.r << 16) | (color.g << 8) | color.b;
                        else
                            pixels[tx + ty * width] = 0xFF000000;
                    }
                }
            }
        }

        xadvance += TEXT_COLS+1;
    }

    SDL_UnlockSurface(surface);
    return surface;
}
