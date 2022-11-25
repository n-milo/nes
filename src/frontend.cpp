#include <SDL2/SDL.h>

#include <sstream>
#include <memory>

#define LOAD_DYNAMICALLY 0      // we want to see typechecking on the declared functions
#include "main.h"

#include "frontend.h"
#include "format.h"
#include "r6502.h"
#include "gfx.h"

namespace {
    constexpr int VIEWPORT_WIDTH = 1400;
    constexpr int VIEWPORT_HEIGHT = 700;

    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;

    constexpr int PADDING = 5;
    constexpr int TEXT_START = 2*PADDING + NES_WIDTH + 10;
    constexpr int INSTRUCTIONS_START = 400;
}

NesFrontend::NesFrontend() : font("monogram-bitmap.json"), bus("roms/nestest.nes") {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("could not init SDL");
    }

    // SDL bug: moving the window to the other monitor freezes the program
    // but if I position the window there to begin with, it works
    // since I want to develop on my 2nd monitor this hack works well for now
    // TODO: remove before release
    int num_displays = SDL_GetNumVideoDisplays();
    std::vector<SDL_Rect> displays(num_displays);
    for (int i = 0; i < num_displays; i++) {
        SDL_GetDisplayBounds(i, &displays[i]);
    }

    int x = displays.back().x + 200;
    int y = displays.back().y + 200;

    window = SDL_CreateWindow("NES", x, y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT, 0);
    if (!window) {
        panic("could not create window");
    }

    window_surface = SDL_GetWindowSurface(window);
    ASSERT(window_surface, "expected window surface");

    init_cpu();
    disassembly = R6502::disassemble(bus, 0x8000, 0xd000);

    last_time = SDL_GetPerformanceCounter();

    printf("Frontend initialized successfully.\n");
}

NesFrontend::~NesFrontend() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void NesFrontend::init_cpu() {
    bus.reset();
}

bool NesFrontend::update() {
    uint64 now = SDL_GetPerformanceCounter();
    float delta = (float) (now-last_time) / (float) SDL_GetPerformanceFrequency();
    last_time = now;

    if (full_speed) {
        // TODO: timing so it goes at 60 Hz
        bus.execute_one_frame();
        frames++;
        frame_time += delta;
        if (frame_time >= 1.0f) {
            frame_time -= 1.0f;
            printf("%d fps\n", frames);
            frames = 0;
        }
    }

    SDL_FillRect(window_surface, nullptr, 0);

    render_cpu();

    auto screen = bus.ppu.render_screen();
    SDL_Rect dst = {PADDING, PADDING, screen->w, screen->h};
    SDL_BlitSurface(screen, nullptr, window_surface, &dst);

    auto pattern = bus.ppu.render_pattern_table(0, current_palette);
    dst = {PADDING, 250, pattern->w, pattern->h};
    SDL_BlitSurface(pattern, nullptr, window_surface, &dst);

    pattern = bus.ppu.render_pattern_table(1, current_palette);
    dst = {PADDING+130, 250, pattern->w, pattern->h};
    SDL_BlitSurface(pattern, nullptr, window_surface, &dst);

    for (int i = 0; i < 8; i++) {
        auto palette = bus.ppu.render_palette(i);
        dst = {PADDING + i*20, 380, pattern->w, pattern->h};
        SDL_BlitSurface(palette, nullptr, window_surface, &dst);
    }

    SDL_UpdateWindowSurface(window);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return true;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                return true;
            } else if (event.key.keysym.sym == SDLK_SPACE) {
                full_speed = !full_speed;
            } else if (event.key.keysym.sym == SDLK_r) {
                bus.reset();
            } else if (event.key.keysym.sym == SDLK_p) {
                current_palette = (current_palette+1) % 8;
            }

            if (!full_speed) {
                if (event.key.keysym.sym == SDLK_c) {
                    bus.clock();
                } else if (event.key.keysym.sym == SDLK_n) {
                    bus.execute_one_instruction();
                } else if (event.key.keysym.sym == SDLK_f) {
                    bus.execute_one_frame();
                }
            }
            break;
        }
    }

    return false;
}

void NesFrontend::render_text(int x, int y, std::string_view text, SDL_Color color) const {
    font.render_to_surface(window_surface, x, y, text, color);
}

void NesFrontend::render_cpu() {
    auto &cpu = bus.cpu;
    auto status = status_to_string(cpu.status);

    // render cpu status
    render_text(TEXT_START, 5,
                string_printf("Status = %02x = %s", cpu.status, status.c_str()));

    render_text(TEXT_START, 25,
                string_printf("A = %02x, X = %02x, Y = %02x", cpu.a, cpu.x, cpu.y));

    render_text(TEXT_START, 45,
                string_printf("SP = %02x, PC = %04x", cpu.sp, cpu.pc));

    render_text(TEXT_START, 65,
                string_printf("Cycles = %d", cpu.cycles));

    // render instructions
    int y = 0;
    render_text(5, INSTRUCTIONS_START+(y++)*20, "C = clock once");
    render_text(5, INSTRUCTIONS_START+(y++)*20, "N = step once");
    render_text(5, INSTRUCTIONS_START+(y++)*20, "F = render once");
    render_text(5, INSTRUCTIONS_START+(y++)*20, "SPACE = start/stop");
    render_text(5, INSTRUCTIONS_START+(y++)*20, "R = reset");
    render_text(5, INSTRUCTIONS_START+(y++)*20, "P = change palette");

    // render disassembly
    auto render_disassembly = [&](int y) {
        auto current_instruction = disassembly.find(cpu.pc);
        if (current_instruction == disassembly.end()) {
            render_text(TEXT_START, y, "???");
            render_text(TEXT_START, y+20, "Could not find:");
            render_text(TEXT_START, y+40, "  disassembly[pc]");
            return;
        }

        // render the 10 instructions above the current one
        auto prev = current_instruction;
        for (int i = 0; i < 10; i++) {
            if (prev == disassembly.begin())
                break;
            --prev;
            render_text(TEXT_START, y + (9-i)*20, string_printf("$%04x: %s", prev->first, prev->second.c_str()), white);
        }

        // render the current instruction
        render_text(TEXT_START, y + 10*20, string_printf("$%04x: %s", current_instruction->first, current_instruction->second.c_str()), red);

        // render the next 10 instructions
        auto next = current_instruction;
        for (int i = 0; i < 10; i++) {
            ++next;
            if (next == disassembly.end())
                break;
            render_text(TEXT_START, y + (i+11)*20, string_printf("$%04x: %s", next->first, next->second.c_str()), white);
        }
    };
    render_disassembly(100);

    auto render_memory = [&](uint16 start_addr, const char *name, int x, int y) {
        render_text(x, y, name);
        for (int mem_y = 0; mem_y < 16; mem_y++) {
            uint16 addr = (mem_y << 4) + start_addr;
            std::string row = string_printf("$%04x: ", addr);
            row.reserve(row.capacity() + 16 * 3); // we want space for 16 words, plus the space in between them
            for (int mem_x = 0; mem_x < 16; mem_x++) {
                row += string_printf("%02x ", bus.read(addr));
                addr++;
            }

            render_text(x, mem_y * 20 + y + 20, row);
        }
    };

    render_memory(0x0000, "Zero page:", TEXT_START + 350, 5);
    render_memory(0x8000, "$8000:", TEXT_START + 350, 350);
}

extern "C" {

void *frontend_init() {
    auto frontend = new NesFrontend;
    return frontend;
}

bool frontend_update(void *arg) {
    auto frontend = reinterpret_cast<NesFrontend *>(arg);
    return frontend->update();
}

void frontend_close(void *arg) {
    auto frontend = reinterpret_cast<NesFrontend *>(arg);
    delete frontend;
}

} // extern "C"