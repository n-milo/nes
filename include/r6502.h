#pragma once

#include <ostream>
#include <optional>
#include <map>
#include <string>

#include "common.h"

class Bus;

enum AddrMode {
    ACC,
    IMM,
    ABS,
    ZP0,
    ZPX,
    ZPY,
    ABX,
    ABY,
    IMP,
    REL,
    IZX,
    IZY,
    IND
};

enum Op {
    ADC, AND, ASL, BCC,
    BCS, BEQ, BIT, BMI,
    BNE, BPL, BRK, BVC,
    BVS, CLC, CLD, CLI,
    CLV, CMP, CPX, CPY,
    DEC, DEX, DEY, EOR,
    INC, INX, INY, JMP,
    JSR, LDA, LDX, LDY,
    LSR, NOP, ORA, PHA,
    PHP, PLA, PLP, ROL,
    ROR, RTI, RTS, SBC,
    SEC, SED, SEI, STA,
    STX, STY, TAX, TAY,
    TSX, TXA, TXS, TYA,

    XXX // Unknown
};

const char *op_to_string(Op op);

struct Instruction {
    Op opcode;
    AddrMode addr_mode;
    uint8 cycle_count;
};

enum Flags {
    C = 1,
    Z = 2,
    I = 4,
    D = 8,
    B = 16,
    U = 32,
    V = 64,
    N = 128,
};

std::string status_to_string(uint8 status);

class R6502 {

    void do_interrupt(Bus &bus, uint16 start_addr);

    /// Calculates the absolute address given the processor state and a given addressing mode.
    /// @param bus the bus
    /// @param addr_mode the addressing mode to use. Must not be ACC, IMM, IMP, or REL.
    /// @param page_crossed out-paramater stating whether or not a page was crossed
    /// @see https://www.nesdev.org/wiki/CPU_addressing_modes
    /// @returns the absolute address given by this addressing mode
    uint16 calculate_address(Bus &bus, AddrMode addr_mode, bool &page_crossed);

    /// Calculate the result of an operation. May modify the processor state.
    /// @param bus the bus
    /// @param op the operation to take
    /// @param operand the 8-bit operand this instruction will use (may be 0 for, e.g. JMP instructions)
    /// @param addr the (optional) absolute address as given by `calculate_address`.
    ///             Will be nullopt if the addressing mode of this instruction is not a valid one as stated given
    ///             by `calculate_address`.
    /// @returns whether or not we need a new cycle if addressing crossed a page
    bool calculate_operation(Bus &bus, Op op, uint8 operand, std::optional<uint16> addr);

    void do_branch(bool condition, int8 relative_addr);
    uint8 do_addition(uint8 src_reg, uint8 operand);

    uint8 read(Bus &bus, uint16 addr);
    void write(Bus &bus, uint16 addr, uint8 data);

public:
    // internal processor registers
    uint8 a = 0, x = 0, y = 0, sp = 0, status = 0;
    uint16 pc = 0;

    uint8 cycles = 0; // cycles left in current instruction
    uint8 last_executed_opcode = 0;
    bool finished_instruction = false;

    // signals for the processor
    bool clock(Bus &bus);
    void reset(Bus &bus);
    void irq(Bus &bus);
    void nmi(Bus &bus);

    static std::map<uint16, std::string> disassemble(Bus &bus, uint16 start, uint16 end);

    void set_flag(Flags flag, bool set) {
        if (set)
            status |= flag;
        else
            status &= ~flag;
    }
};