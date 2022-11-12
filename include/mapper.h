#pragma once

#include <optional>

class Mapper {
protected:
    int num_prgs, num_chrs;

public:
    Mapper(int num_prgs, int num_chrs) : num_prgs(num_prgs), num_chrs(num_chrs) {}
    virtual ~Mapper() = default;

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
            return addr & (num_prgs > 1 ? 0x7fff : 0x3fff);
        }
        return {};
    }

    std::optional<uint32> map_cpu_write(uint16 addr) override {
        if (addr >= 0x8000 && addr <= 0xffff) {
            return addr & (num_prgs > 1 ? 0x7fff : 0x3fff);
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

