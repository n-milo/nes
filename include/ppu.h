#pragma once

#include <SDL2/SDL.h>
#include "cartridge.h"

#define PATTERN_START 0x0000
#define PATTERN_END   0x1FFF

#define NAMETABLE_START 0x2000
#define NAMETABLE_END   0x3EFF

#define PALETTE_START 0x3F00
#define PALETTE_END   0x3FFF

class PPU {
    Cartridge *cartridge;
    uint8 name_table_mem[2048] = {};
    uint8 pattern_mem[8192] = {};
    uint8 palette_mem[32] = {};

    union {
        uint8 value = 0;
        struct {
            uint8 :5, sprite_overflow:1, sprite_zero_hit:1, vertical_blank:1;
        };
    } status;

    union {
        uint8 value = 0;
        struct {
            uint8 grayscale:1;
            uint8 show_background_left:1; // 1: Show background in leftmost 8 pixels of screen, 0: Hide
            uint8 show_sprites_left:1;    // 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
            uint8 show_background:1;
            uint8 show_sprites:1;
            uint8 emphasize_red:1;
            uint8 emphasize_green:1;
            uint8 emphasize_blue:1;
        };
    } mask;

    union {
        uint8 value = 0;
        struct {
            uint8 base_nametable_addr:2;     // Base nametable address
                                             // (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
            uint8 vram_addr_increment:1;     // VRAM address increment per CPU read/write of PPUDATA
                                             // (0: add 1, going across; 1: add 32, going down)
            uint8 sprite_pattern_addr:1;     // Sprite pattern table address for 8x8 sprites
                                             // (0: $0000; 1: $1000; ignored in 8x16 mode)
            uint8 background_pattern_addr:1; // Background pattern table address (0: $0000; 1: $1000)
            uint8 sprite_size:1;             // Sprite size (0: 8x8 pixels; 1: 8x16 pixels – see PPU OAM#Byte 1)
            uint8 master:1;                  // PPU master/slave select
                                             // (0: read backdrop from EXT pins; 1: output color on EXT pins)
            uint8 nmi_on_vblank:1;           // Generate an NMI at the start of the vertical blanking interval
                                             // (0: off; 1: on)
        };
    } control;

    bool next_address_is_lsb = false; // see https://www.nesdev.org/wiki/PPU_registers#PPUADDR
    uint8 data_buffer = 0;
    uint16 combined_address = 0;

    int scanline = 0;
    int cycle = 0;

    SDL_Surface *screen;
    SDL_Surface *pattern_tables[2] = {};
    SDL_Surface *palettes[8] = {};

public:
    bool finished_frame = false;

    explicit PPU(Cartridge *cartridge);

    void ppu_write(uint16 addr, uint8 data);
    uint8 ppu_read(uint16 addr);
    void cpu_write(uint16 addr, uint8 data);
    uint8 cpu_read(uint16 addr);

    SDL_Surface *render_screen();
    SDL_Surface *render_pattern_table(int table, uint8 palette);
    SDL_Surface *render_palette(int palette);

    SDL_Color color_from_palette(uint8 palette, uint8 pixel);

    void clock(bool &nmi_requested);
};