#include "r6502.h"

#include <sstream>
#include <magic_enum.hpp>

#include "bus.h"

extern const Instruction instruction_lookup_table[256];

bool R6502::clock() {
    if (cycles == 0) {
        uint8 opcode = read(pc++);
        last_executed_opcode = opcode;
        auto &instr = instruction_lookup_table[opcode];
        cycles = instr.cycle_count;

        bool page_crossed = false;

        uint8 operand;
        std::optional<uint16> addr;

        switch (instr.addr_mode) {

        case ACC: // instruction operates on the accumulator
            operand = a;
            break;

        case IMM: // instruction is immediate
        case REL: // instruction is relative
            // relative instructions and immediate instructions are kind of the same thing in my implementation
            // (both have an 8-bit operand immediately following the instruction)
            // for relative instructions, the addition is handled by the operation execution logic, not the
            // addressing logic
            operand = read(pc++);
            break;

        case IMP: // instruction operand is implied (e.g. RTS)
            operand = a;
            break;

        default:
            addr = calculate_address(instr.addr_mode, page_crossed);
            operand = read(*addr);
            break;

        }

        bool needs_cycle_for_computing = calculate_operation(instr.opcode, operand, addr);
        if (page_crossed && needs_cycle_for_computing)
            cycles++;
    }

    cycles--;
    return cycles == 0;
}

void R6502::reset() {
    a = x = y = 0;
    sp = 0xFD;
    status = U;

    uint16 lo = read(0xFFFC);
    uint16 hi = read(0xFFFD);
    pc = (hi << 8) | lo;

    cycles = 8;
}

void R6502::irq() {
    if (status & I) {
        do_interrupt(0xFFFE);
        cycles = 7;
    }
}

void R6502::nmi() {
    do_interrupt(0xFFFA);
    cycles = 8;
}

void R6502::do_interrupt(uint16 vector) {
    write(0x100 + sp--, pc >> 8);
    write(0x100 + sp--, pc & 0xFF);

    status &= ~B;
    status |= (U | I);
    write(0x100 + sp--, status);

    uint16 lo = read(vector);
    uint16 hi = read(vector + 1);
    pc = (hi << 8) | lo;
}

uint16 R6502::calculate_address(AddrMode addr_mode, bool &page_crossed) {
    // https://www.nesdev.org/wiki/CPU_addressing_modes

    page_crossed = false;

    switch (addr_mode) {

    case ACC: // instruction operates on the accumulator
    case IMM: // instruction is immediate
    case IMP: // instruction operand is implied (e.g. RTS)
    case REL: // instruction is relative
        // all of these cases are already handled in the clock() code, so really this part should be unreachable
        panic("addressing mode `%s` does not have a real address",
              std::string(magic_enum::enum_name(addr_mode)).c_str());

    case ABS: { // absolute addressing (read 2 bytes for address)
        uint16 lo = read(pc++);
        uint16 hi = read(pc++);
        return (hi << 8) | lo;
    }

    case ZP0: // zero-page addressing
        return read(pc++);

    case ZPX: // zero-page + x addressing
        return (read(pc++) + x) & 0xFF;

    case ZPY: // zero-page + y addressing
        return (read(pc++) + y) & 0xFF;

    case ABX: { // absolute addressing + x
        uint16 lo = read(pc++);
        uint16 hi = read(pc++);
        uint16 addr = ((hi << 8) | lo) + x;
        if ((addr & 0xFF00) != (hi << 8)) {
            // if the +x causes a carry-out on lo, we have crossed a page
            page_crossed = true;
        }
        return addr;
    }

    case ABY: { // absolute addressing + y
        uint16 lo = read(pc++);
        uint16 hi = read(pc++);
        uint16 addr = ((hi << 8) | lo) + y;
        if ((addr & 0xFF00) != (hi << 8)) {
            // same as above
            page_crossed = true;
        }
        return addr;
    }

    case IND: { // read a 16-bit pointer at a 16-bit absolute address, then 16-bit value at that address
        uint16 lo = read(pc++);
        uint16 hi = read(pc++);
        uint16 ptr_addr = ((hi << 8) | lo);

        // 6502 has a bug
        // If we try to read a pointer starting at, e.g. $13FF, the +1 won't carry to the high byte, and we'll end
        // up creating the 16-bit pointer out of $13FF and $1300. This code simulates that bug.
        uint8 ind_hi;
        if (lo == 0xFF)
            ind_hi = read(ptr_addr & 0xFF00);
        else
            ind_hi = read(ptr_addr + 1);

        uint8 ind_lo = read(ptr_addr);
        return (ind_hi << 8) | ind_lo;
    }

    case IZX: { // read an 8-bit pointer to the zero page and add x
        uint16 ptr_addr = read(pc++);
        uint16 ind_lo = read((ptr_addr + x) & 0xFF);
        uint16 ind_hi = read((ptr_addr + x + 1) & 0xFF);
        return (ind_hi << 8) | ind_lo;
    }

    case IZY: { // read a 16-bit pointer (in the zero page) and add y to it
        uint16 ptr_addr = read(pc++);
        uint16 lo = read(ptr_addr);
        uint16 hi = read((ptr_addr + 1) & 0xFF);
        uint16 addr = ((hi << 8) | lo) + y;
        if ((addr & 0xFF00) != (hi << 8)) {
            // same as above
            page_crossed = true;
        }
        return addr;
    }

    }
}

bool R6502::calculate_operation(Op op,
                                uint8 operand,
                                std::optional<uint16> addr) {

    uint8 result;

    switch (op) {

    // loads and stores

    case LDA:
        a = operand;
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return true;

    case LDX:
        x = operand;
        set_flag(Z, x == 0);
        set_flag(N, x & 0x80);
        return true;

    case LDY:
        y = operand;
        set_flag(Z, y == 0);
        set_flag(N, y & 0x80);
        return true;

    case STA: // STore Accumulator
        ASSERT(addr.has_value(), "STA must have an address");
        write(*addr, a);
        return false;

    case STX: // STore X register
        ASSERT(addr.has_value(), "STX must have an address");
        write(*addr, x);
        return false;

    case STY: // STore Y register
        ASSERT(addr.has_value(), "STY must have an address");
        write(*addr, y);
        return false;

    // transfers

    case TAX:
        x = a;
        return false;
    case TAY:
        y = a;
        return false;
    case TSX:
        x = sp;
        return false;
    case TXA:
        a = x;
        return false;
    case TXS:
        return false;
    case TYA:
        a = y;
        return false;

    // stack

    case PHA: // PusH Accumulator
        write(0x100 + sp--, a);
        return false;

    case PHP:
        write(0x100 + sp--, status);
        return false;

    case PLA:
        a = read(0x100 + ++sp);
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return false;

    case PLP:
        status = read(0x100 + ++sp);
        return false;

    // shift

    case ASL: // Arithmetic Shift Left
        set_flag(C, operand & 0x80); // bit 7 is shifted into carry
        result = operand << 1;
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        if (addr.has_value())
            write(*addr, result);
        else // if addr is nullopt, the target must be the accumulator
            a = result;
        return false;

    case LSR: // Logical Shift Right
        set_flag(C, operand & 0x1); // bit 0 is shifted into carry
        result = operand >> 1;
        set_flag(Z, result == 0);
        set_flag(N, false); // 0 goes into bit 7 so result can never be negative
        if (addr.has_value())
            write(*addr, result);
        else
            a = result;
        return false;

    case ROL: // ROtate Left
        result = operand << 1;
        result |= (status & C) ? 1 : 0; // carry is shifted into bit 0
        set_flag(C, operand & 0x80); // the original bit 7 is shifted into the carry
        if (addr.has_value())
            write(*addr, result);
        else
            a = result;
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        return false;

    case ROR: // ROtate Right
        result = operand >> 1;
        result |= (status & C) ? (1 << 7) : 0; // carry is shifted into bit 7
        set_flag(C, operand & 0x1); // the original bit 0 is shifted into the carry
        if (addr.has_value())
            write(*addr, result);
        else
            a = result;
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        return false;

    // logic

    case AND: // bitwise AND with accumulator
        a &= operand;
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return true;

    case BIT: // test BITs
        result = a & operand;
        set_flag(Z, result == 0);
        set_flag(N, operand & 0x80);
        set_flag(V, operand & 0x40);
        return false;

    case EOR:
        a ^= operand;
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return true;

    case ORA: // bitwise OR with Accumulator
        a |= operand;
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return true;

    // arith

    case ADC: // ADd with Carry
        a = do_addition(a, operand);
        return true;

    case CMP: // CoMPare accumulator
        set_flag(Z, a == operand);
        set_flag(N, ((uint16)a - (uint16)operand) & 0x80);
        set_flag(C, operand <= a);
        return true;

    case CPX:
        set_flag(Z, x == operand);
        set_flag(N, ((uint16)x - (uint16)operand) & 0x80);
        set_flag(C, operand <= x);
        return false;

    case CPY:
        set_flag(Z, y == operand);
        set_flag(N, ((uint16)y - (uint16)operand) & 0x80);
        set_flag(C, operand <= y);
        return false;

    case SBC: // SuBtract with Carry
        // for subtraction, just flip the bits in the operand and fallthrough to the addition code
        a = do_addition(a, ~operand);
        return true;

    // inc/dec

    case DEC:
        ASSERT(addr.has_value(), "DEC must have an address");
        result = operand - 1;
        write(*addr, result);
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        return false;

    case DEX:
        x--;
        set_flag(Z, x == 0);
        set_flag(N, x & 0x80);
        return false;

    case DEY:
        y--;
        set_flag(Z, y == 0);
        set_flag(N, y & 0x80);
        return false;

    case INC:
        ASSERT(addr.has_value(), "INC must have an address");
        result = operand + 1;
        write(*addr, result);
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        return false;

    case INX:
        x++;
        set_flag(Z, x == 0);
        set_flag(N, x & 0x80);
        return false;

    case INY:
        y++;
        set_flag(Z, y == 0);
        set_flag(N, y & 0x80);
        return false;

    // control

    case BRK:
        irq();
        TODO("BRK");

    case JMP:
        ASSERT(addr.has_value(), "JMP must have an address");
        pc = *addr;
        return false;

    case JSR:
        pc--;
        write(0x100 + sp--, pc >> 8);
        write(0x100 + sp--, pc & 0xFF);
        pc = *addr;
        return false;

    case RTI:
        status = read(0x100 + ++sp);
        status &= ~(B | U);
        pc = read(0x100 + ++sp);
        pc |= read(0x100 + ++sp) << 8;
        return false;

    case RTS: // ReTurn from Subroutine
        pc = read(0x100 + ++sp);
        pc |= read(0x100 + ++sp) << 8;
        pc++;
        return false;

    // branch

    case BCC: // Branch on Carry Clear
        do_branch(!(status & C), static_cast<int8>(operand));
        return false;
    case BCS: // Branch on Carry Set
        do_branch(status & C, static_cast<int8>(operand));
        return false;
    case BEQ: // Branch on EQual
        do_branch(status & Z, static_cast<int8>(operand));
        return false;
    case BMI: // Branch on MInus
        do_branch(status & N, static_cast<int8>(operand));
        return false;
    case BNE: // Branch on Not Equal
        do_branch(!(status & Z), static_cast<int8>(operand));
        return false;
    case BPL: // Branch on PLus
        do_branch(!(status & N), static_cast<int8>(operand));
        return false;
    case BVC: // Branch on oVerflow Clear
        do_branch(!(status & V), static_cast<int8>(operand));
        return false;
    case BVS: // Branch on oVerflow Set
        do_branch(status & V, static_cast<int8>(operand));
        return false;

    // flag control

    case CLC:
        status &= ~C;
        return false;
    case CLD:
        status &= ~D;
        return false;
    case CLI:
        status &= ~I;
        return false;
    case CLV:
        status &= ~V;
        return false;

    case SEC:
        set_flag(C, true);
        return false;
    case SED:
        set_flag(D, true);
        return false;
    case SEI:
        set_flag(I, true);
        return false;

    // nop

    case NOP: // No OPeration
        return false;

    default:
    case XXX:
        fprintf(stderr, "illegal instruction at 0x%x", pc);
        __builtin_trap();
    }
}

void R6502::do_branch(bool condition, int8 relative_addr) {
    if (condition) {
        cycles++; // add a cycle if we branch

        // interpret the operand as the signed 8-bit address offset
        uint16 addr = pc + relative_addr;

        // add a cycle if we cross a page
        if ((addr & 0xFF00) != (pc & 0xFF00))
            cycles++;

        pc = addr;
    }
}

uint8 R6502::do_addition(uint8 src_reg, uint8 operand) {
    uint16 carry = (status & C) ? 1 : 0;

    // perform addition in 16 bits so we don't overflow on carry
    uint16 result = (uint16)src_reg + (uint16)operand + carry;

    // set carry if the top bit is 1
    set_flag(C, result & 0x100);

    if (!(src_reg & 0x80) && !(operand & 0x80) && (result & 0x80)) {
        // If src_reg is positive and the operand is positive, but our result is negative, we have
        // overflowed. Set the overflow flag.
        set_flag(V, true);
    } else if ((src_reg & 0x80) && (operand & 0x80) && !(result & 0x80)) {
        // If a is negative (bit 7 is 1) and the operand is negative, but our result is positive, we also overflow.
        set_flag(V, true);
    } else {
        // no overflow, reset the flag
        set_flag(V, false);
    }

    uint8 ret = result & 0xFF;
    set_flag(Z, ret == 0);
    set_flag(N, ret & 0x80);
    return ret;
}

uint8 R6502::read(uint16 addr) {
    ASSERT(bus, "bus must be connected");
    return bus->read(addr);
}

void R6502::write(uint16 addr, uint8 data) {
    ASSERT(bus, "bus must be connected");
    bus->write(addr, data);
}

std::map<uint16, std::string> R6502::disassemble(uint16 start, uint16 end) {
    std::map<uint16, std::string> strings;

    uint16 addr = start;
    while (addr <= end) {
        std::stringstream ss;

        uint16 line_start = addr;
        uint8 opcode = read(addr++);
        auto &instr = instruction_lookup_table[opcode];
        ss << magic_enum::enum_name(instr.opcode) << " ";

        int hi, lo;
        char buf[256] = {};

        switch (instr.addr_mode) {

        case ACC:
            buf[0] = 'A';
            break;
        case IMM:
            snprintf(buf, sizeof buf, "#$%02x", read(addr++));
            break;
        case ABS:
            lo = read(addr++);
            hi = read(addr++);
            snprintf(buf, sizeof buf, "$%04x", (lo | (hi << 8)));
            break;
        case ZP0:
            snprintf(buf, sizeof buf, "$%02x", read(addr++));
            break;
        case ZPX:
            snprintf(buf, sizeof buf, "$%02x,X", read(addr++));
            break;
        case ZPY:
            snprintf(buf, sizeof buf, "$%02x,Y", read(addr++));
            break;
        case ABX:
            lo = read(addr++);
            hi = read(addr++);
            snprintf(buf, sizeof buf, "$%04x,X", (lo | (hi << 8)));
            break;
        case ABY:
            lo = read(addr++);
            hi = read(addr++);
            snprintf(buf, sizeof buf, "$%04x,Y", (lo | (hi << 8)));
            break;
        case IMP:
            break;
        case REL:
            lo = read(addr++);
            snprintf(buf, sizeof buf, "$%02x [$%04x]", lo, addr + static_cast<int8>(lo));
            break;
        case IZX:
            snprintf(buf, sizeof buf, "($%02x,X)", read(addr++));
            break;
        case IZY:
            snprintf(buf, sizeof buf, "($%02x),Y", read(addr++));
            break;
        case IND:
            lo = read(addr++);
            hi = read(addr++);
            snprintf(buf, sizeof buf, "($%04x)", (lo | (hi << 8)));
            break;
        }

        ss << std::string(buf);
        strings[line_start] = ss.str();
    }

    return strings;
}

const Instruction instruction_lookup_table[256] = {
        { BRK, IMM, 7 },{ ORA, IZX, 6 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 3 },{ ORA, ZP0, 3 },{ ASL, ZP0, 5 },{ XXX, IMP, 5 },{ PHP, IMP, 3 },{ ORA, IMM, 2 },{ ASL, IMP, 2 },{ XXX, IMP, 2 },{ NOP, IMP, 4 },{ ORA, ABS, 4 },{ ASL, ABS, 6 },{ XXX, IMP, 6 },
        { BPL, REL, 2 },{ ORA, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ ORA, ZPX, 4 },{ ASL, ZPX, 6 },{ XXX, IMP, 6 },{ CLC, IMP, 2 },{ ORA, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ ORA, ABX, 4 },{ ASL, ABX, 7 },{ XXX, IMP, 7 },
        { JSR, ABS, 6 },{ AND, IZX, 6 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ BIT, ZP0, 3 },{ AND, ZP0, 3 },{ ROL, ZP0, 5 },{ XXX, IMP, 5 },{ PLP, IMP, 4 },{ AND, IMM, 2 },{ ROL, IMP, 2 },{ XXX, IMP, 2 },{ BIT, ABS, 4 },{ AND, ABS, 4 },{ ROL, ABS, 6 },{ XXX, IMP, 6 },
        { BMI, REL, 2 },{ AND, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ AND, ZPX, 4 },{ ROL, ZPX, 6 },{ XXX, IMP, 6 },{ SEC, IMP, 2 },{ AND, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ AND, ABX, 4 },{ ROL, ABX, 7 },{ XXX, IMP, 7 },
        { RTI, IMP, 6 },{ EOR, IZX, 6 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 3 },{ EOR, ZP0, 3 },{ LSR, ZP0, 5 },{ XXX, IMP, 5 },{ PHA, IMP, 3 },{ EOR, IMM, 2 },{ LSR, IMP, 2 },{ XXX, IMP, 2 },{ JMP, ABS, 3 },{ EOR, ABS, 4 },{ LSR, ABS, 6 },{ XXX, IMP, 6 },
        { BVC, REL, 2 },{ EOR, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ EOR, ZPX, 4 },{ LSR, ZPX, 6 },{ XXX, IMP, 6 },{ CLI, IMP, 2 },{ EOR, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ EOR, ABX, 4 },{ LSR, ABX, 7 },{ XXX, IMP, 7 },
        { RTS, IMP, 6 },{ ADC, IZX, 6 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 3 },{ ADC, ZP0, 3 },{ ROR, ZP0, 5 },{ XXX, IMP, 5 },{ PLA, IMP, 4 },{ ADC, IMM, 2 },{ ROR, IMP, 2 },{ XXX, IMP, 2 },{ JMP, IND, 5 },{ ADC, ABS, 4 },{ ROR, ABS, 6 },{ XXX, IMP, 6 },
        { BVS, REL, 2 },{ ADC, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ ADC, ZPX, 4 },{ ROR, ZPX, 6 },{ XXX, IMP, 6 },{ SEI, IMP, 2 },{ ADC, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ ADC, ABX, 4 },{ ROR, ABX, 7 },{ XXX, IMP, 7 },
        { NOP, IMP, 2 },{ STA, IZX, 6 },{ NOP, IMP, 2 },{ XXX, IMP, 6 },{ STY, ZP0, 3 },{ STA, ZP0, 3 },{ STX, ZP0, 3 },{ XXX, IMP, 3 },{ DEY, IMP, 2 },{ NOP, IMP, 2 },{ TXA, IMP, 2 },{ XXX, IMP, 2 },{ STY, ABS, 4 },{ STA, ABS, 4 },{ STX, ABS, 4 },{ XXX, IMP, 4 },
        { BCC, REL, 2 },{ STA, IZY, 6 },{ XXX, IMP, 2 },{ XXX, IMP, 6 },{ STY, ZPX, 4 },{ STA, ZPX, 4 },{ STX, ZPY, 4 },{ XXX, IMP, 4 },{ TYA, IMP, 2 },{ STA, ABY, 5 },{ TXS, IMP, 2 },{ XXX, IMP, 5 },{ NOP, IMP, 5 },{ STA, ABX, 5 },{ XXX, IMP, 5 },{ XXX, IMP, 5 },
        { LDY, IMM, 2 },{ LDA, IZX, 6 },{ LDX, IMM, 2 },{ XXX, IMP, 6 },{ LDY, ZP0, 3 },{ LDA, ZP0, 3 },{ LDX, ZP0, 3 },{ XXX, IMP, 3 },{ TAY, IMP, 2 },{ LDA, IMM, 2 },{ TAX, IMP, 2 },{ XXX, IMP, 2 },{ LDY, ABS, 4 },{ LDA, ABS, 4 },{ LDX, ABS, 4 },{ XXX, IMP, 4 },
        { BCS, REL, 2 },{ LDA, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 5 },{ LDY, ZPX, 4 },{ LDA, ZPX, 4 },{ LDX, ZPY, 4 },{ XXX, IMP, 4 },{ CLV, IMP, 2 },{ LDA, ABY, 4 },{ TSX, IMP, 2 },{ XXX, IMP, 4 },{ LDY, ABX, 4 },{ LDA, ABX, 4 },{ LDX, ABY, 4 },{ XXX, IMP, 4 },
        { CPY, IMM, 2 },{ CMP, IZX, 6 },{ NOP, IMP, 2 },{ XXX, IMP, 8 },{ CPY, ZP0, 3 },{ CMP, ZP0, 3 },{ DEC, ZP0, 5 },{ XXX, IMP, 5 },{ INY, IMP, 2 },{ CMP, IMM, 2 },{ DEX, IMP, 2 },{ XXX, IMP, 2 },{ CPY, ABS, 4 },{ CMP, ABS, 4 },{ DEC, ABS, 6 },{ XXX, IMP, 6 },
        { BNE, REL, 2 },{ CMP, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ CMP, ZPX, 4 },{ DEC, ZPX, 6 },{ XXX, IMP, 6 },{ CLD, IMP, 2 },{ CMP, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ CMP, ABX, 4 },{ DEC, ABX, 7 },{ XXX, IMP, 7 },
        { CPX, IMM, 2 },{ SBC, IZX, 6 },{ NOP, IMP, 2 },{ XXX, IMP, 8 },{ CPX, ZP0, 3 },{ SBC, ZP0, 3 },{ INC, ZP0, 5 },{ XXX, IMP, 5 },{ INX, IMP, 2 },{ SBC, IMM, 2 },{ NOP, IMP, 2 },{ SBC, IMP, 2 },{ CPX, ABS, 4 },{ SBC, ABS, 4 },{ INC, ABS, 6 },{ XXX, IMP, 6 },
        { BEQ, REL, 2 },{ SBC, IZY, 5 },{ XXX, IMP, 2 },{ XXX, IMP, 8 },{ NOP, IMP, 4 },{ SBC, ZPX, 4 },{ INC, ZPX, 6 },{ XXX, IMP, 6 },{ SED, IMP, 2 },{ SBC, ABY, 4 },{ NOP, IMP, 2 },{ XXX, IMP, 7 },{ NOP, IMP, 4 },{ SBC, ABX, 4 },{ INC, ABX, 7 },{ XXX, IMP, 7 },
};
