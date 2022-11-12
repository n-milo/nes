#include "bus.h"

void Bus::write(uint16 addr, uint8 data) {
    if (addr >= RAM_START && addr <= RAM_END) {
        ram[addr % 0x7ff] = data;
    } else if (addr >= ROM_START && addr <= ROM_END) {
        rom[addr - ROM_START] = data;
    } else {
        printf("warning: invalid write to %04x", addr);
    }
}

uint8 Bus::read(uint16 addr) const {
    if (addr >= RAM_START && addr <= RAM_END) {
        return ram[addr % 0x7ff];
    } else if (addr >= ROM_START && addr <= ROM_END) {
        return rom[addr - ROM_START];
    } else {
        printf("warning: invalid read from %04x", addr);
        return 0;
    }
}
