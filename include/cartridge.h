#pragma once

#include <string_view>
#include <vector>

#include "mapper.h"

class Cartridge {

    std::vector<uint8> prg, chr;
    int num_prgs, num_chrs;

    std::unique_ptr<Mapper> mapper;

public:
    explicit Cartridge(const char *file);

    std::optional<uint8> cpu_read(uint16 addr);
    bool cpu_write(uint16 addr, uint8 val);
    std::optional<uint8> ppu_read(uint16 addr);
    bool ppu_write(uint16 addr, uint8 val);

};