#include <ppu.h>
#include <gfx.h>
#include <font.h>

extern SDL_Color palette_array[64];

PPU::PPU(Cartridge *cartridge) : cartridge(cartridge) {
    screen = gfx::create_surface(256, 240);
    pattern_tables[0] = gfx::create_surface(128, 128);
    pattern_tables[1] = gfx::create_surface(128, 128);
    for (int i = 0; i < 8; i++)
        palettes[i] = gfx::create_surface(16, 4);
    ASSERT(screen, "failed to create surface");
}

uint8 *PPU::ppu_locate(uint16 addr) {
    if (addr >= PATTERN_START && addr <= PATTERN_END) {
        LOG_WARN("ppu address %04x should have mapped by the cartridge", addr);
        return nullptr;
    } else if (addr >= NAMETABLE_START && addr <= NAMETABLE_END) {
        addr &= 0xfff;
        switch (cartridge->mirroring) {

        case Mirroring::Vertical:
            if (addr >= 0 && addr <= 0x3ff)
                return &name_table_mem[0][addr & 0x3ff];
            else if (addr >= 0x400 && addr <= 0x7ff)
                return &name_table_mem[1][addr & 0x3ff];
            else if (addr >= 0x800 && addr <= 0xbff)
                return &name_table_mem[0][addr & 0x3ff];
            else if (addr >= 0xc00 && addr <= 0xfff)
                return &name_table_mem[1][addr & 0x3ff];
            else
                UNREACHABLE("ranges above covers all addresses");

        case Mirroring::Horizontal:
            if (addr >= 0 && addr <= 0x3ff)
                return &name_table_mem[0][addr & 0x3ff];
            else if (addr >= 0x400 && addr <= 0x7ff)
                return &name_table_mem[0][addr & 0x3ff];
            else if (addr >= 0x800 && addr <= 0xbff)
                return &name_table_mem[1][addr & 0x3ff];
            else if (addr >= 0xc00 && addr <= 0xfff)
                return &name_table_mem[1][addr & 0x3ff];
            else
                UNREACHABLE("ranges above covers all addresses");
        }
    } else if (addr >= PALETTE_START && addr <= PALETTE_END) {
        addr &= 0x1f;
        if (addr == 0x10) addr = 0x0;
        if (addr == 0x14) addr = 0x4;
        if (addr == 0x18) addr = 0x8;
        if (addr == 0x1c) addr = 0xc;
        return &palette_mem[addr];
    } else {
        return nullptr;
    }
}

void PPU::ppu_write(uint16 addr, uint8 data) {
    if (cartridge->ppu_write(addr, data)) {
        return;
    } else {
        uint8 *target = ppu_locate(addr);
        if (target)
            *target = data;
    }
}

uint8 PPU::ppu_read(uint16 addr) {
    if (auto data = cartridge->ppu_read(addr); data.has_value()) {
        return *data;
    } else {
        uint8 *target = ppu_locate(addr);
        if (target)
            return *target;
        else
            return 0;
    }
}

void PPU::cpu_write(uint16 addr, uint8 data) {
    switch (addr) {

    case 0:
        control.value = data;
        break;
    case 1:
        mask.value = data;
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;

    case 6:
        if (next_address_is_lsb) {
            combined_address = (combined_address & 0xFF00) | data;
            next_address_is_lsb = false;
        } else {
            combined_address = (combined_address & 0xFF) | (data << 8);
            next_address_is_lsb = true;
        }
        break;

    case 7:
        ppu_write(combined_address, data);
//        combined_address += (control.vram_addr_increment ? 32 : 1);
        break;

    default:
        break;

    }
}

uint8 PPU::cpu_read(uint16 addr) {
    switch (addr) {

    case 0:
        break;
    case 1:
        break;
    case 2: {
        uint8 ret = (status.value & 0xe0) | (data_buffer & 0x1f);
        status.vertical_blank = 0;
        next_address_is_lsb = false;
        return ret;
    }
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;

    case 6:
        break;

    case 7: {
        // all reads except the palette memory are delayed by one frame
        uint8 data = data_buffer;
        data_buffer = ppu_read(combined_address);
        if (combined_address >= 0x3f00)
            data = data_buffer;

        combined_address += (control.vram_addr_increment ? 32 : 1);
        return data;
    }

    default:
        return 0;

    }

    return 0;
}

SDL_Surface *PPU::render_screen() {
    return screen;
}

SDL_Surface *PPU::render_pattern_table(int table, uint8 palette) {
    SDL_Surface *surface = pattern_tables[table];
    SDL_LockSurface(surface);

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int tile_index = y * 16 + x;

            for (int row = 0; row < 8; row++) {
                uint8 tile_lsb = ppu_read(table * 0x1000 + tile_index * 16 + row);
                uint8 tile_msb = ppu_read(table * 0x1000 + tile_index * 16 + row + 8);

                for (int col = 0; col < 8; col++) {
                    int shift = 7-col;
                    int bit = 1 << shift; // most-significant bit is left-most pixel
                    uint8 pixel = ((tile_lsb & bit) >> shift) + ((tile_msb & bit) >> shift);

                    int pixel_x = x * 8 + col;
                    int pixel_y = y * 8 + row;
                    auto color = color_from_palette(palette, pixel);
                    gfx::set_pixel(surface, pixel_x, pixel_y, color);
                }
            }
        }
    }

    SDL_UnlockSurface(surface);
    return surface;
}

SDL_Surface *PPU::render_palette(int palette) {
    auto surface = palettes[palette];
    SDL_LockSurface(surface);
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 16; x++) {
            int color_index = x / 4;
            auto color = color_from_palette((uint8) palette, (uint8) color_index);
            gfx::set_pixel(surface, x, y, color);
        }
    }
    SDL_UnlockSurface(surface);
    return surface;
}

SDL_Color PPU::color_from_palette(uint8 palette, uint8 pixel) {
    uint8 index = ppu_read(0x3f00 + (palette << 2) + pixel);
    return palette_array[index % 64];
}

void PPU::clock(bool &nmi_requested) {
    if (scanline == -1 && cycle == 1) {
        status.vertical_blank = 0;
    } else if (scanline == 241 && cycle == 1) {
        status.vertical_blank = 1;
        if (control.nmi_on_vblank) {
            nmi_requested = true;
        }
    }

    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
            finished_frame = true;
        }
    }
}

SDL_Color palette_array[64] = {
        { 84,  84,  84},  {  0,  30, 116},  {  8,  16, 144},  { 48,   0, 136},  { 68,   0, 100},  { 92,   0,  48},  { 84,   4,   0},  { 60,  24,   0},  { 32,  42,   0},  {  8,  58,   0},  {  0,  64,   0},  {  0,  60,   0},  {  0,  50,  60},  {  0,   0,   0},
        {152, 150, 152},  {  8,  76, 196},  { 48,  50, 236},  { 92,  30, 228},  {136,  20, 176},  {160,  20, 100},  {152,  34,  32},  {120,  60,   0},  { 84,  90,   0},  { 40, 114,   0},  {  8, 124,   0},  {  0, 118,  40},  {  0, 102, 120},  {  0,   0,   0},
        {236, 238, 236},  { 76, 154, 236},  {120, 124, 236},  {176,  98, 236},  {228,  84, 236},  {236,  88, 180},  {236, 106, 100},  {212, 136,  32},  {160, 170,   0},  {116, 196,   0},  { 76, 208,  32},  { 56, 204, 108},  { 56, 180, 204},  { 60,  60,  60},
        {236, 238, 236},  {168, 204, 236},  {188, 188, 236},  {212, 178, 236},  {236, 174, 236},  {236, 174, 212},  {236, 180, 176},  {228, 196, 144},  {204, 210, 120},  {180, 222, 120},  {168, 226, 144},  {152, 226, 180},  {160, 214, 228},  {160, 162, 160},
};