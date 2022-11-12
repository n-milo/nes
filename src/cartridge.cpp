#include "cartridge.h"

#include <fstream>

Cartridge::Cartridge(const char *file) {
    // https://www.nesdev.org/wiki/INES

    struct iNESHeader {
        char magic[4];
        uint8 prg_size;
        uint8 chr_size;
        uint8 flags6;
        uint8 flags7;
        uint8 prg_ram_size;
        uint8 tv_system;
        uint8 flags10;
        char padding[5];
    };

    static_assert(sizeof(iNESHeader) == 16, "iNES header is 16 bytes");

    std::ifstream in(file);
    in.open(file, std::ifstream::binary);
    ASSERT(in.is_open(), "%s does not exist", file);

    iNESHeader header;
    in.read(reinterpret_cast<char *>(&header), 16);

    if (header.flags6 & 0x4) {
        in.seekg(512, std::ios_base::cur);
    }

    uint8 mapper_number = (header.flags6 >> 4) | (header.flags7 & 0xf0);
    switch (mapper_number) {

    case 0:
        mapper = std::make_unique<Mapper00NROM>(num_prgs, num_chrs);
        break;

    default:
        panic("mapped %d not yet implemented", mapper_number);

    }
}

std::optional<uint8> Cartridge::cpu_read(uint16 addr) {
    auto mapped = mapper->map_cpu_read(addr);
    if (mapped.has_value())
        return prg[*mapped];
    else
        return {};

}

std::optional<uint8> Cartridge::ppu_read(uint16 addr) {
    auto mapped = mapper->map_ppu_read(addr);
    if (mapped.has_value())
        return chr[*mapped];
    else
        return {};
}

bool Cartridge::cpu_write(uint16 addr, uint8 val) {
    auto mapped = mapper->map_cpu_read(addr);
    if (mapped.has_value()) {
        prg[*mapped] = val;
        return true;
    } else
        return false;
}

bool Cartridge::ppu_write(uint16 addr, uint8 val) {
    auto mapped = mapper->map_ppu_read(addr);
    if (mapped.has_value()) {
        chr[*mapped] = val;
        return true;
    } else
        return false;
}
