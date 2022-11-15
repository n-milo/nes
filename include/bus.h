#pragma once

#include <vector>
#include "common.h"
#include "r6502.h"

#define RAM_START 0
#define RAM_END   0x1FFF

#define ROM_START 0x4020
#define ROM_END   0xFFFF

class Bus {
public:
    std::vector<uint8> ram;
    std::vector<uint8> rom;

    Bus() : ram(2 * 1024), rom(ROM_END - ROM_START + 1)
        {}

    ~Bus() = default;

    void write(uint16 addr, uint8 data);
    uint8 read(uint16 addr) const;
};
