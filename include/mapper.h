#pragma once

#include <optional>

class Mapper {
protected:
    int num_prg_banks, num_chr_banks;

public:
    Mapper(int num_prg_banks, int num_chr_banks) : num_prg_banks(num_prg_banks), num_chr_banks(num_chr_banks) {}
    virtual ~Mapper() = default;

    // The family of map_*_* functions map an address to a new index into the cartridge's prg and chr vectors.
    // Some addresses won't be mapped. E.g. 0x1000 points to the NES RAM, so we shouldn't touch it and mappers should
    // return an empty optional. Some addresses, however, such as 0x9000 are not mapped by the CPU and are then
    // mapped by the mapper onto somewhere into the prg vector in the cartridge.

    virtual std::optional<uint32> map_cpu_read(uint16 addr) = 0;
    virtual std::optional<uint32> map_cpu_write(uint16 addr) = 0;
    virtual std::optional<uint32> map_ppu_read(uint16 addr) = 0;
    virtual std::optional<uint32> map_ppu_write(uint16 addr) = 0;
};

class Mapper00NROM : public Mapper {
public:
    Mapper00NROM(int num_prgs, int num_chrs) : Mapper(num_prgs, num_chrs) {}

    std::optional<uint32> map_cpu_read(uint16 addr) override {
        if (addr >= 0x8000 && addr <= 0xffff) {
            return addr & (num_prg_banks > 1 ? 0x7fff : 0x3fff);
        }
        return {};
    }

    std::optional<uint32> map_cpu_write(uint16 addr) override {
        if (addr >= 0x8000 && addr <= 0xffff) {
            return addr & (num_prg_banks > 1 ? 0x7fff : 0x3fff);
        }
        return {};
    }

    std::optional<uint32> map_ppu_read(uint16 addr) override {
        if (addr >= 0x8000 && addr <= 0xFFFF) {
            return addr;
        }
        return {};
    }

    std::optional<uint32> map_ppu_write(uint16 addr) override {
        // if the ppu is writing it must be to the pattern memory, but you can't write to that
        return {};
    }
};

