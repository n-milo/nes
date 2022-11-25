#pragma once

#include <vector>

#include "r6502.h"
#include "ppu.h"
#include "cartridge.h"

#define RAM_START 0
#define RAM_END   0x1FFF

#define PPU_START 0x2000
#define PPU_END   0x3FFF

#define CONTROLLER_START 0x4016
#define CONTROLLER_END   0x4017

class Bus {
public:
    Cartridge cartridge;
    R6502 cpu;
    PPU ppu;

    uint64 system_clock = 0;

    std::vector<uint8> ram;

    uint8 controller[2] = {};
    uint8 controller_saved_state[2] = {};

    explicit Bus(const char *file) : Bus(Cartridge::load_cartridge(file)) {}

    explicit Bus(Cartridge &&cartridge)
        : cartridge(std::move(cartridge))
        , ram(2 * 1024)
        , ppu(&this->cartridge)
    {}

    ~Bus() = default;

    Bus(Bus&&) = delete;
    Bus& operator=(Bus&&) = delete;

    void write(uint16 addr, uint8 data);
    uint8 read(uint16 addr);

    void clock();
    void execute_one_instruction();
    void execute_one_frame();
    void reset();
};
