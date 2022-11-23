#pragma once

#include <string_view>
#include <vector>

#include "mapper.h"

class Cartridge {
    int num_prg_banks, num_chr_banks;
    std::unique_ptr<Mapper> mapper;

public:
    std::vector<uint8> prg, chr;

    Cartridge(int num_prg_banks, int num_chr_banks, std::unique_ptr<Mapper> &&mapper)
        : prg(num_prg_banks * 16*1024)
        , chr(num_chr_banks * 8*1024)
        , num_prg_banks(num_prg_banks)
        , num_chr_banks(num_chr_banks)
        , mapper(std::move(mapper))
    {}

    static Cartridge load_cartridge(const char *file);

    std::optional<uint8> cpu_read(uint16 addr);
    bool cpu_write(uint16 addr, uint8 val);
    std::optional<uint8> ppu_read(uint16 addr);
    bool ppu_write(uint16 addr, uint8 val);

};