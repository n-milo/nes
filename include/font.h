#pragma once

#include <SDL2/SDL.h>

#include <map>
#include <array>
#include <string>
#include <string_view>

class Font {
public:
    static Font &the_font() {
        static Font font;
        return font;
    }

    static constexpr int TEXT_COLS = 5;
    static constexpr int TEXT_ROWS = 12;

private:
    // each font character is 12 5-bit bitfields corresponding to whether or not each pixel in a row is set
    // (LSB is leftmost)
    std::map<std::string, std::array<uint8, TEXT_ROWS>> characters;

public:
    Font() = default;
    explicit Font(const char *bitmap_json);

    SDL_Surface *render_text(std::string_view text, SDL_Color color, int scale = 2) const;
    void render_to_surface(SDL_Surface *surface, int x, int y, std::string_view text, SDL_Color color, int scale = 2) const;
};