#pragma once

#include <array>

class PPU {
    uint8 name_table[2048] = {};
    uint8 palette[32] = {};

public:
    void ppu_write(uint16 addr, uint8 data);
    uint8 ppu_read(uint16 addr) const;
    void cpu_write(uint16 addr, uint8 data);
    uint8 cpu_read(uint16 addr) const;
};