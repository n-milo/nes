#pragma once

#include <vector>
#include "common.h"
#include "r6502.h"

class Bus {

public:
    R6502 cpu;
    std::vector<uint8> ram;

    Bus() : ram(64 * 1024)
        {}

    ~Bus() = default;

    void write(uint16 addr, uint8 data);
    uint8 read(uint16 addr);
};