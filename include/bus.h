#pragma once

#include <vector>

#include "r6502.h"
#include "ppu.h"
#include "cartridge.h"

#define RAM_START 0
#define RAM_END   0x1FFF

#define PPU_START 0x2000
#define PPU_END   0x3FFF

class Bus {
public:
    Cartridge cartridge;
    R6502 cpu;
    PPU ppu;

    std::vector<uint8> ram;

    explicit Bus(Cartridge &&cartridge)
        : cartridge(std::move(cartridge))
        , ram(2 * 1024)
    {}

    ~Bus() = default;

    void write(uint16 addr, uint8 data);
    uint8 read(uint16 addr);

    void clock();
    void reset();
};
