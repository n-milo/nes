#pragma once

#include <SDL2/SDL.h>
#include "font.h"
#include "bus.h"

constexpr SDL_Color white = {255, 255, 255, 255};
constexpr SDL_Color red = {255, 127, 127, 255};

enum class ScreenVisualization {
    Display,
    NametableID,
    Patterns0,
    Patterns1,
};

ScreenVisualization& operator++(ScreenVisualization &v) {
    v = static_cast<ScreenVisualization>((static_cast<int>(v) + 1) % 4);
    return v;
}

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
    bool breakpoints_enabled = false;
    int frames = 0;
    float frame_time = 0;
    int current_palette = 0;
    ScreenVisualization visualization = ScreenVisualization::Display;

    NesFrontend();
    ~NesFrontend();

    void init_cpu();
    bool update();
    void render_text(int x, int y, std::string_view text, SDL_Color color = white) const;
    void render_cpu();
};