#include "bus.h"

void Bus::write(uint16 addr, uint8 data) {
    if (cartridge.cpu_write(addr, data)) {
        // TODO
    } else if (addr >= RAM_START && addr <= RAM_END) {
        ram[addr & 0x7ff] = data;
    } else if (addr >= PPU_START && addr <= PPU_END) {
        ppu.cpu_write(addr & 0x7, data);
    } else {
        fprintf(stderr, "warning: invalid write to %04x\n", addr);
    }
}

uint8 Bus::read(uint16 addr) {
    if (auto data = cartridge.cpu_read(addr); data.has_value()) {
        return *data;
    } else if (addr >= RAM_START && addr <= RAM_END) {
        return ram[addr & 0x7ff];
    } else if (addr >= PPU_START && addr <= PPU_END) {
        return ppu.cpu_read(addr & 0x7);
    } else {
        fprintf(stderr, "warning: invalid read from %04x\n", addr);
        return 0;
    }
}

void Bus::clock() {
    cpu.clock(*this);
}

void Bus::reset() {
    cpu.reset(*this);
}

void Bus::execute_one_instruction() {
    cpu.finished_instruction = false;
    while (!cpu.finished_instruction)
        clock();
}
