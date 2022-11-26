#include "r6502.h"

#include <sstream>

#include "bus.h"

extern const Instruction instruction_lookup_table[256];

void R6502::clock(Bus &bus) {
    // set the pc back to its initial value on a breakpoint
    uint16 saved_pc = pc;
    defer {
        if (std::uncaught_exceptions() > 0) {
            pc = saved_pc;
        }
    };

    if (cycles == 0) {
        finished_instruction = true;

        uint16 trace_addr = pc;
        LOG_TRACE("executing $%04x: %s", pc, R6502::disassemble_instruction(bus, trace_addr).c_str());

        uint8 opcode = read(bus, pc++);
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
            operand = read(bus, pc++);
            break;

        case IMP: // instruction operand is implied (e.g. RTS)
            operand = a;
            break;

        default:
            addr = calculate_address(bus, instr.addr_mode, page_crossed);
            if (instr.opcode == STA || instr.opcode == STX || instr.opcode == STY) {
                // store instructions shouldn't read
                // all other instructions will actually read from the address
                operand = 0;
            } else {
                operand = read(bus, *addr);
            }
            break;

        }

        bool needs_cycle_for_computing = calculate_operation(bus, instr.opcode, operand, addr);
        if (page_crossed && needs_cycle_for_computing)
            cycles++;
    }

    cycles--;
}

void R6502::reset(Bus &bus) noexcept {
    bool saved_breakpoints_enabled = breakpoints_enabled;
    breakpoints_enabled = false;
    defer { breakpoints_enabled = saved_breakpoints_enabled; };

    a = x = y = 0;
    sp = 0xFD;
    status = U;

    uint16 lo = read(bus, 0xFFFC);
    uint16 hi = read(bus, 0xFFFD);
    pc = (hi << 8) | lo;

    cycles = 8;
}

void R6502::irq(Bus &bus) noexcept {
    if (status & I) {
        do_interrupt(bus, 0xFFFE);
        cycles = 7;
    }
}

void R6502::nmi(Bus &bus) noexcept {
    do_interrupt(bus, 0xFFFA);
    cycles = 8;
}

void R6502::do_interrupt(Bus &bus, uint16 vector) {
    bool saved_breakpoints_enabled = breakpoints_enabled;
    breakpoints_enabled = false;
    defer { breakpoints_enabled = saved_breakpoints_enabled; };

    write(bus, 0x100 + sp--, pc >> 8);
    write(bus, 0x100 + sp--, pc & 0xFF);

    status &= ~B;
    status |= (U | I);
    write(bus, 0x100 + sp--, status);

    uint16 lo = read(bus, vector);
    uint16 hi = read(bus, vector + 1);
    pc = (hi << 8) | lo;
}

uint16 R6502::calculate_address(Bus &bus, AddrMode addr_mode, bool &page_crossed) {
    // https://www.nesdev.org/wiki/CPU_addressing_modes

    page_crossed = false;

    switch (addr_mode) {

    case ACC: // instruction operates on the accumulator
    case IMM: // instruction is immediate
    case IMP: // instruction operand is implied (e.g. RTS)
    case REL: // instruction is relative
        // all of these cases are already handled in the clock() code, so really this part should be unreachable
        panic("addressing mode does not have a real address");

    case ABS: { // absolute addressing (read 2 bytes for address)
        uint16 lo = read(bus, pc++);
        uint16 hi = read(bus, pc++);
        return (hi << 8) | lo;
    }

    case ZP0: // zero-page addressing
        return read(bus, pc++);

    case ZPX: // zero-page + x addressing
        return (read(bus, pc++) + x) & 0xFF;

    case ZPY: // zero-page + y addressing
        return (read(bus, pc++) + y) & 0xFF;

    case ABX: { // absolute addressing + x
        uint16 lo = read(bus, pc++);
        uint16 hi = read(bus, pc++);
        uint16 addr = ((hi << 8) | lo) + x;
        if ((addr & 0xFF00) != (hi << 8)) {
            // if the +x causes a carry-out on lo, we have crossed a page
            page_crossed = true;
        }
        return addr;
    }

    case ABY: { // absolute addressing + y
        uint16 lo = read(bus, pc++);
        uint16 hi = read(bus, pc++);
        uint16 addr = ((hi << 8) | lo) + y;
        if ((addr & 0xFF00) != (hi << 8)) {
            // same as above
            page_crossed = true;
        }
        return addr;
    }

    case IND: { // read a 16-bit pointer at a 16-bit absolute address, then 16-bit value at that address
        uint16 lo = read(bus, pc++);
        uint16 hi = read(bus, pc++);
        uint16 ptr_addr = ((hi << 8) | lo);

        // 6502 has a bug
        // If we try to read a pointer starting at, e.g. $13FF, the +1 won't carry to the high byte, and we'll end
        // up creating the 16-bit pointer out of $13FF and $1300. This code simulates that bug.
        uint8 ind_hi;
        if (lo == 0xFF)
            ind_hi = read(bus, ptr_addr & 0xFF00);
        else
            ind_hi = read(bus, ptr_addr + 1);

        uint8 ind_lo = read(bus, ptr_addr);
        return (ind_hi << 8) | ind_lo;
    }

    case IZX: { // read an 8-bit pointer to the zero page and add x
        uint16 ptr_addr = read(bus, pc++);
        uint16 ind_lo = read(bus, (ptr_addr + x) & 0xFF);
        uint16 ind_hi = read(bus, (ptr_addr + x + 1) & 0xFF);
        return (ind_hi << 8) | ind_lo;
    }

    case IZY: { // read a 16-bit pointer (in the zero page) and add y to it
        uint16 ptr_addr = read(bus, pc++);
        uint16 lo = read(bus, ptr_addr);
        uint16 hi = read(bus, (ptr_addr + 1) & 0xFF);
        uint16 addr = ((hi << 8) | lo) + y;
        if ((addr & 0xFF00) != (hi << 8)) {
            // same as above
            page_crossed = true;
        }
        return addr;
    }

    }
}

bool R6502::calculate_operation(Bus &bus,
                                Op op,
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
        write(bus, *addr, a);
        return false;

    case STX: // STore X register
        ASSERT(addr.has_value(), "STX must have an address");
        write(bus, *addr, x);
        return false;

    case STY: // STore Y register
        ASSERT(addr.has_value(), "STY must have an address");
        write(bus, *addr, y);
        return false;

    // transfers

    case TAX:
        x = a;
        set_flag(N, x & 0x80);
        set_flag(Z, x == 0);
        return false;
    case TAY:
        y = a;
        set_flag(N, y & 0x80);
        set_flag(Z, y == 0);
        return false;
    case TSX:
        x = sp;
        set_flag(N, x & 0x80);
        set_flag(Z, x == 0);
        return false;
    case TXA:
        a = x;
        set_flag(N, a & 0x80);
        set_flag(Z, a == 0);
        return false;
    case TXS:
        sp = x;
        return false;
    case TYA:
        a = y;
        set_flag(N, a & 0x80);
        set_flag(Z, a == 0);
        return false;

    // stack

    case PHA: // PusH Accumulator
        write(bus, 0x100 + sp--, a);
        return false;

    case PHP:
        write(bus, 0x100 + sp--, status | B | U);
        set_flag(B, false);
        set_flag(U, false);
        return false;

    case PLA:
        a = read(bus, 0x100 + ++sp);
        set_flag(Z, a == 0);
        set_flag(N, a & 0x80);
        return false;

    case PLP:
        status = read(bus, 0x100 + ++sp);
        return false;

    // shift

    case ASL: // Arithmetic Shift Left
        set_flag(C, operand & 0x80); // bit 7 is shifted into carry
        result = operand << 1;
        set_flag(Z, result == 0);
        set_flag(N, result & 0x80);
        if (addr.has_value())
            write(bus, *addr, result);
        else // if addr is nullopt, the target must be the accumulator
            a = result;
        return false;

    case LSR: // Logical Shift Right
        set_flag(C, operand & 0x1); // bit 0 is shifted into carry
        result = operand >> 1;
        set_flag(Z, result == 0);
        set_flag(N, false); // 0 goes into bit 7 so result can never be negative
        if (addr.has_value())
            write(bus, *addr, result);
        else
            a = result;
        return false;

    case ROL: // ROtate Left
        result = operand << 1;
        result |= (status & C) ? 1 : 0; // carry is shifted into bit 0
        set_flag(C, operand & 0x80); // the original bit 7 is shifted into the carry
        if (addr.has_value())
            write(bus, *addr, result);
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
            write(bus, *addr, result);
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
        write(bus, *addr, result);
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
        write(bus, *addr, result);
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
//        irq(bus);
        TODO("BRK");

    case JMP:
        ASSERT(addr.has_value(), "JMP must have an address");
        pc = *addr;
        return false;

    case JSR:
        pc--;
        write(bus, 0x100 + sp--, pc >> 8);
        write(bus, 0x100 + sp--, pc & 0xFF);
        pc = *addr;
        return false;

    case RTI:
        status = read(bus, 0x100 + ++sp);
        status &= ~(B | U);
        pc = read(bus, 0x100 + ++sp);
        pc |= read(bus, 0x100 + ++sp) << 8;
        return false;

    case RTS: // ReTurn from Subroutine
        pc = read(bus, 0x100 + ++sp);
        pc |= read(bus, 0x100 + ++sp) << 8;
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

    default:
    case NOP: // No OPeration
    case XXX:
        return false;
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

uint8 R6502::read(Bus &bus, uint16 addr) {
    if (breakpoints_enabled && std::find(address_read_breakpoints.begin(), address_read_breakpoints.end(), addr) != address_read_breakpoints.end()) {
        throw BreakpointException{};
    }
    return bus.read(addr);
}

void R6502::write(Bus &bus, uint16 addr, uint8 data) {
    if (breakpoints_enabled && std::find(address_write_breakpoints.begin(), address_write_breakpoints.end(), addr) != address_write_breakpoints.end()) {
        throw BreakpointException{};
    }
    bus.write(addr, data);
}

std::map<uint16, std::string> R6502::disassemble(Bus &bus, uint16 start, uint16 end) {
    std::map<uint16, std::string> strings;

    uint16 addr = start;
    while (addr <= end) {
        uint16 line_start = addr;
        auto disassembled = R6502::disassemble_instruction(bus, addr);
        strings[line_start] = disassembled;
    }

    return strings;
}

std::string R6502::disassemble_instruction(Bus &bus, uint16 &addr) {
    uint8 opcode = bus.read(addr++);
    auto &instr = instruction_lookup_table[opcode];

    auto disassembled = std::string(op_to_string(instr.opcode));

    int hi, lo;
    char buf[256] = {};

    switch (instr.addr_mode) {

    case ACC:
        buf[0] = 'A';
        break;
    case IMM:
        snprintf(buf, sizeof buf, "#$%02x", bus.read(addr++));
        break;
    case ABS:
        lo = bus.read(addr++);
        hi = bus.read(addr++);
        snprintf(buf, sizeof buf, "$%04x", (lo | (hi << 8)));
        break;
    case ZP0:
        snprintf(buf, sizeof buf, "$%02x", bus.read(addr++));
        break;
    case ZPX:
        snprintf(buf, sizeof buf, "$%02x,X", bus.read(addr++));
        break;
    case ZPY:
        snprintf(buf, sizeof buf, "$%02x,Y", bus.read(addr++));
        break;
    case ABX:
        lo = bus.read(addr++);
        hi = bus.read(addr++);
        snprintf(buf, sizeof buf, "$%04x,X", (lo | (hi << 8)));
        break;
    case ABY:
        lo = bus.read(addr++);
        hi = bus.read(addr++);
        snprintf(buf, sizeof buf, "$%04x,Y", (lo | (hi << 8)));
        break;
    case IMP:
        break;
    case REL:
        lo = bus.read(addr++);
        snprintf(buf, sizeof buf, "$%02x [$%04x]", lo, addr + static_cast<int8>(lo));
        break;
    case IZX:
        snprintf(buf, sizeof buf, "($%02x,X)", bus.read(addr++));
        break;
    case IZY:
        snprintf(buf, sizeof buf, "($%02x),Y", bus.read(addr++));
        break;
    case IND:
        lo = bus.read(addr++);
        hi = bus.read(addr++);
        snprintf(buf, sizeof buf, "($%04x)", (lo | (hi << 8)));
        break;
    }

    if (buf[0] != 0) {
        disassembled += " ";
        disassembled += buf;
    }

    return disassembled;
}

/*
    C = 1,
    Z = 2,
    I = 4,
    D = 8,
    B = 16,
    U = 32,
    V = 64,
    N = 128,
 */

std::string status_to_string(uint8 status) {
    std::stringstream ss;
    if (status & C)
        ss << "C | ";
    if (status & Z)
        ss << "Z | ";
    if (status & I)
        ss << "I | ";
    if (status & D)
        ss << "D | ";
    if (status & B)
        ss << "B | ";
    if (status & U)
        ss << "U | ";
    if (status & V)
        ss << "V | ";
    if (status & N)
        ss << "N | ";

    auto str = ss.str();
    if (!str.empty()) {
        // crop off the last "| " that would be added
        str.erase(str.end()-1);
        str.erase(str.end()-1);
    }

    return str;
}

const char *op_to_string(Op op) {
    switch (op) {
        case ADC: return "ADC";
        case AND: return "AND";
        case ASL: return "ASL";
        case BCC: return "BCC";
        case BCS: return "BCS";
        case BEQ: return "BEQ";
        case BIT: return "BIT";
        case BMI: return "BMI";
        case BNE: return "BNE";
        case BPL: return "BPL";
        case BRK: return "BRK";
        case BVC: return "BVC";
        case BVS: return "BVS";
        case CLC: return "CLC";
        case CLD: return "CLD";
        case CLI: return "CLI";
        case CLV: return "CLV";
        case CMP: return "CMP";
        case CPX: return "CPX";
        case CPY: return "CPY";
        case DEC: return "DEC";
        case DEX: return "DEX";
        case DEY: return "DEY";
        case EOR: return "EOR";
        case INC: return "INC";
        case INX: return "INX";
        case INY: return "INY";
        case JMP: return "JMP";
        case JSR: return "JSR";
        case LDA: return "LDA";
        case LDX: return "LDX";
        case LDY: return "LDY";
        case LSR: return "LSR";
        case NOP: return "NOP";
        case ORA: return "ORA";
        case PHA: return "PHA";
        case PHP: return "PHP";
        case PLA: return "PLA";
        case PLP: return "PLP";
        case ROL: return "ROL";
        case ROR: return "ROR";
        case RTI: return "RTI";
        case RTS: return "RTS";
        case SBC: return "SBC";
        case SEC: return "SEC";
        case SED: return "SED";
        case SEI: return "SEI";
        case STA: return "STA";
        case STX: return "STX";
        case STY: return "STY";
        case TAX: return "TAX";
        case TAY: return "TAY";
        case TSX: return "TSX";
        case TXA: return "TXA";
        case TXS: return "TXS";
        case TYA: return "TYA";

        default:
        case XXX: return "???";
    }
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
