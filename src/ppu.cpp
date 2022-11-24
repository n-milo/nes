#include <ppu.h>
#include <gfx.h>

extern SDL_Color palette_array[64];

PPU::PPU(Cartridge *cartridge)
    : cartridge(cartridge)
    , screen_surface(SDL_CreateRGBSurface(0, 256, 240, 32, 0, 0, 0, 0))
{
    ASSERT(screen_surface, "failed to create surface");
}

void PPU::ppu_write(uint16 addr, uint8 data) {
    if (cartridge->ppu_write(addr, data)) {
        return;
    } else if (addr >= PATTERN_START && addr <= PATTERN_END) {
        fprintf(stderr, "warning: ppu address %04x should have mapped by the cartridge\n", addr);
    } else if (addr >= NAMETABLE_START && addr <= NAMETABLE_END) {

    } else if (addr >= PALETTE_START && addr <= PALETTE_END) {
        addr &= 0x1f;
        if (addr == 0x10) addr = 0x0;
        if (addr == 0x14) addr = 0x4;
        if (addr == 0x18) addr = 0x8;
        if (addr == 0x1c) addr = 0xc;
        palette_mem[addr] = data;
    } else {
        fprintf(stderr, "warning: invalid write to %04x\n", addr);
    }
}

uint8 PPU::ppu_read(uint16 addr) {
    if (auto data = cartridge->ppu_read(addr); data.has_value()) {
        return *data;
    } else if (addr >= PATTERN_START && addr <= PATTERN_END) {
        fprintf(stderr, "warning: ppu address %04x should have mapped by the cartridge\n", addr);
    } else if (addr >= NAMETABLE_START && addr <= NAMETABLE_END) {

    } else if (addr >= PALETTE_START && addr <= PALETTE_END) {
        addr &= 0x1f;
        if (addr == 0x10) addr = 0x0;
        if (addr == 0x14) addr = 0x4;
        if (addr == 0x18) addr = 0x8;
        if (addr == 0x1c) addr = 0xc;
        return palette_mem[addr];
    } else {
        fprintf(stderr, "warning: invalid read from %04x\n", addr);
    }
    return 0;
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
        status.vertical_blank = 1; // hack
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

    case 7:
        if (combined_address > 0x3f00) {
            return data_buffer;
        } else {
            // all reads except the palette memory are delayed by one frame
            uint8 old = data_buffer;
            data_buffer = ppu_read(combined_address);
            return old;
        }

    default:
        return 0;

    }

    return 0;
}

SDL_Surface *PPU::create_pattern_table(int table, uint8 palette) {
    SDL_Surface *surface = SDL_CreateRGBSurface(0, 256, 240, 32, 0, 0, 0, 0);
    auto pixels = reinterpret_cast<uint32 *>(surface);

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int tile_index = y * 16 + x;

            for (int row = 0; row < 8; row++) {
                uint8 tile_lsb = ppu_read(table * 0x1000 + tile_index * 16 + row);
                uint8 tile_msb = ppu_read(table * 0x1000 + tile_index * 16 + row + 8);

                for (int col = 0; col < 8; col++) {
                    int bit = 1 << (7 - col); // most-significant bit is left-most pixel
                    uint8 pixel = (tile_lsb & bit) + (tile_msb & bit);

                    int pixel_x = x * 8 + col;
                    int pixel_y = y * 8 + row;
                    auto color = color_from_palette(palette, pixel);
                    pixels[pixel_x + pixel_y * surface->w] = (color.r << 16) | (color.g << 8) | color.b;
                }
            }
        }
    }

    return nullptr;
}

SDL_Color PPU::color_from_palette(uint8 palette, uint8 pixel) {
    uint8 index = ppu_read(0x3f00 + (palette << 2) + pixel);
    return palette_array[index];
}

void PPU::clock() {
    int x = cycle-1;
    int y = scanline;
    if (gfx::in_bounds(screen_surface, x, y)) {
        SDL_LockSurface(screen_surface);
        gfx::set_pixel(screen_surface, x, y, palette_array[rand() % 64]);
        SDL_UnlockSurface(screen_surface);
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