#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <sstream>

#include "format.h"
#include "bus.h"
#include "r6502.h"

namespace {
    constexpr int VIEWPORT_WIDTH = 1400;
    constexpr int VIEWPORT_HEIGHT = 700;

    constexpr int NES_WIDTH = 256;
    constexpr int NES_HEIGHT = 240;

    constexpr int PADDING = 5;
    constexpr int TEXT_START = 2*PADDING + NES_WIDTH;
}

TTF_Font *font;
SDL_Color white = {255, 255, 255};
SDL_Color blue = {255, 127, 127};

void render_text(SDL_Surface *window_surface, int x, int y, const char *str, SDL_Color color = white) {
    SDL_Surface *message = TTF_RenderText_Solid(font, str, color);
    SDL_Rect dst = {x, y, message->w, message->h};
    SDL_BlitSurface(message, nullptr, window_surface, &dst);
}

void render_text(SDL_Surface *window_surface, int x, int y, const std::string &str, SDL_Color color = white) {
    render_text(window_surface, x, y, str.c_str(), color);
}

void init_cpu(R6502 &cpu, Bus &bus) {
    std::stringstream program;
    program << "A2 0A 8E 00 00 A2 03 8E 01 00 AC 00 00 A9 00 18 6D 01 00 88 D0 FA 8D 02 00 EA EA EA";

    int addr = 0;
    while (!program.eof()) {
        std::string byte;
        program >> byte;
        uint8 digit = std::stoi(byte, nullptr, 16);
        bus.rom[addr++] = digit;
    }

    // program starts at ROM_START
    bus.write(0xFFFC, ROM_START & 0xFF);
    bus.write(0xFFFD, ROM_START >> 8);

    cpu.reset(bus);
}

void render_cpu(const R6502 &cpu, const Bus &bus, SDL_Surface *window_surface, const std::map<uint16, std::string> &disassembly) {
    auto status = status_to_string(cpu.status);

    // render cpu status
    render_text(window_surface, TEXT_START, 5,
                string_printf("Status = %02x = %s", cpu.status, status.c_str()));

    render_text(window_surface, TEXT_START, 25,
                string_printf("A = %02x, X = %02x, Y = %02x", cpu.a, cpu.x, cpu.y));

    render_text(window_surface, TEXT_START, 45,
                string_printf("SP = %02x, PC = %04x", cpu.sp, cpu.pc));

    render_text(window_surface, TEXT_START, 65,
                string_printf("Cycles = %d", cpu.cycles));

    // render instructions
    render_text(window_surface, 5, 250, "N = clock once");

    // render disassembly
    {
        int y = 100;
        for (auto &[addr, instruction]: disassembly) {
            auto color = (addr == cpu.pc) ? blue : white;
            render_text(window_surface, TEXT_START, y, string_printf("$%04x: %s", addr, instruction.c_str()), color);
            y += 20;
        }
    }

    auto render_memory = [&](uint16 start_addr, const char *name, int x, int y) {
        render_text(window_surface, x, y, name);
        for (int mem_y = 0; mem_y < 16; mem_y++) {
            uint16 addr = (mem_y << 4) + start_addr;
            std::string row = string_printf("$%04x: ", addr);
            row.reserve(row.capacity() + 16 * 3); // we want space for 16 words, plus the space in between them
            for (int mem_x = 0; mem_x < 16; mem_x++) {
                row += string_printf("%02x ", bus.read(addr));
                addr++;
            }

            render_text(window_surface, x, mem_y * 20 + y + 20, row);
        }
    };

    render_memory(0x0000, "Zero page:", TEXT_START + 350, 5);
    render_memory(0x4020, "ROM:", TEXT_START + 350, 350);
}

int main() {
    // TODO: in this file: error handling

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        panic("could not init SDL");
    }

    auto window = SDL_CreateWindow("NES",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   VIEWPORT_WIDTH, VIEWPORT_HEIGHT,
                                   0);
    if (!window) {
        panic("could not create window");
    }

    auto window_surface = SDL_GetWindowSurface(window);
    ASSERT(window_surface, "expected window surface");

    TTF_Init();

    font = TTF_OpenFont("pixel.ttf", 18);
    ASSERT(font, "expected font");

    SDL_Surface *screen = SDL_CreateRGBSurface(0, NES_WIDTH, NES_HEIGHT, 24, 0, 0, 0, 0);
    ASSERT(screen, "expected surface");

    SDL_LockSurface(screen);
    for (int i = 0; i < screen->pitch * screen->h; i++) {
        reinterpret_cast<char *>(screen->pixels)[i] = rand() & 0xFF;
    }
    SDL_UnlockSurface(screen);

    Bus bus;
    R6502 cpu;
    init_cpu(cpu, bus);

    auto disassembly = R6502::disassemble(bus, 0x4020, 0x4040);

    bool running = true;
    while (running) {
        SDL_FillRect(window_surface, nullptr, 0);

        render_cpu(cpu, bus, window_surface, disassembly);

        SDL_Rect dst = {PADDING, PADDING, screen->w, screen->h};
        SDL_BlitSurface(screen, nullptr, window_surface, &dst);

        SDL_UpdateWindowSurface(window);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_n) {
                    cpu.clock(bus);
                }
                break;
            }
        }
    }

    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
