#include "bus.h"

void Bus::write(uint16 addr, uint8 data) {
    if (addr >= 0x0000 && addr <= 0xFFFF) {
        ram[addr] = data;
//        printf("%02x -> [%04x]\n", data, addr);
    } else {
//        printf("%02x -> [%04x] (out of range)\n", data, addr);
    }
}

uint8 Bus::read(uint16 addr) const {
    if (addr >= 0x0000 && addr <= 0xFFFF) {
//        printf("[%04x] -> %02x\n", addr, ram[addr]);
        return ram[addr];
    } else {
//        printf("[%04x] -> out of range\n", addr);
        return 0;
    }
}
