#pragma once

#include <SDL2/SDL.h>
#include "font.h"
#include "bus.h"

constexpr SDL_Color white = {255, 255, 255, 255};
constexpr SDL_Color red = {255, 127, 127, 255};

class NesFrontend {
public:
    SDL_Window *window;
    SDL_Surface *window_surface;
    SDL_Surface *selected_palette_surface;

    Font font;

    Bus bus;
    std::map<uint16, std::string> disassembly;

    uint64 last_time;
    bool full_speed = false;
    int frames = 0;
    float frame_time = 0;
    int current_palette = 0;

    NesFrontend();
    ~NesFrontend();

    void init_cpu();
    bool update();
    void render_text(int x, int y, std::string_view text, SDL_Color color = white) const;
    void render_cpu();
};