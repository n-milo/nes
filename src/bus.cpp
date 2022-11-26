#include "bus.h"

void Bus::write(uint16 addr, uint8 data) {
    LOG_TRACE("[$%04x] <- %02x", addr, data);
    if (cartridge.cpu_write(addr, data)) {
        // TODO
    } else if (addr >= RAM_START && addr <= RAM_END) {
        ram[addr & 0x7ff] = data;
    } else if (addr >= PPU_START && addr <= PPU_END) {
        ppu.cpu_write(addr & 0x7, data);
    } else if (addr >= CONTROLLER_START && addr <= CONTROLLER_END) {
        controller_saved_state[addr & 1] = controller[addr & 1];
    }
}

uint8 Bus::read(uint16 addr) {
    uint8 data = 0;
    std::optional<uint8> cartridge_data = cartridge.cpu_read(addr);

    if (cartridge_data.has_value()) {
        data = *cartridge_data;
    } else if (addr >= RAM_START && addr <= RAM_END) {
        data = ram[addr & 0x7ff];
    } else if (addr >= PPU_START && addr <= PPU_END) {
        data = ppu.cpu_read(addr & 0x7);
    } else if (addr >= CONTROLLER_START && addr <= CONTROLLER_END) {
        data = !!(controller_saved_state[addr & 1] & 0x80);
        controller_saved_state[addr & 1] <<= 1;
    }

    LOG_TRACE("[$%04x] -> %02x", addr, data);
    return data;
}

void Bus::clock() {
    bool nmi_requested = false;
    ppu.clock(nmi_requested);
    if (system_clock % 3 == 0) { // cpu clocks at 1/3rd the rate of the ppu
        cpu.clock(*this);
    }

    if (nmi_requested) {
        cpu.nmi(*this);
    }

    system_clock++;
}

void Bus::reset() {
    bool saved_breakpoints_enabled = breakpoints_enabled;
    breakpoints_enabled = false;
    defer { breakpoints_enabled = saved_breakpoints_enabled; };

    cpu.reset(*this);
}

void Bus::execute_one_instruction() {
    cpu.finished_instruction = false;
    while (!cpu.finished_instruction)
        clock();
}

void Bus::execute_one_frame() {
    ppu.finished_frame = false;
    while (!ppu.finished_frame)
        clock();
}
