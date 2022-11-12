#include <iostream>
#include <fstream>
#include <vector>

#include <catch2/catch_all.hpp>

#include "r6502.h"
#include "bus.h"

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

struct Trace {
    uint16 address;
    uint8 opcode;
    std::string disassembled;
    uint8 x, y, a, sp, status;
    uint16 pc;
    uint64 num_clocks;
};

Bus load_rom(std::string_view path) {
    std::ifstream romfile(path);
    if (!romfile.is_open()) {
        throw std::invalid_argument(std::string(path) + " does not exist");
    }

    std::vector<uint8> rom;
    std::string line;
    while (std::getline(romfile, line)) {
        if (line.rfind("//", 0) == 0 || line.empty()) {
            // line is a comment or blank
            continue;
        }

        uint8 digit = std::stoi(line, nullptr, 16);
        rom.push_back(digit);
    }

    Bus bus;
    std::copy(rom.begin(), rom.end(), bus.ram.begin() + 0xF000);

    return bus;
}

std::vector<Trace> load_trace(std::string_view path) {
    std::ifstream tracefile(path);
    ASSERT(tracefile.is_open(), "file must exist");

    std::vector<Trace> traces;
    std::string line;

    while (std::getline(tracefile, line)) {
        if (line.rfind("TRACE>", 0) != 0 || line.empty()) {
            // line is not a "TRACE>", or it's blank
            continue;
        }

        Trace trace;

        trace.disassembled = line.substr(25, 52-25);
        rtrim(trace.disassembled);
        auto semicolon = trace.disassembled.find(';');
        if (semicolon != std::string::npos) {
            trace.disassembled = trace.disassembled.substr(0, semicolon);
            rtrim(trace.disassembled);
        }

        auto opcode_str = line.substr(12, 2);
        if (opcode_str == "  ") {
            // we have the last line
            break;
        }

        trace.address = std::stoi(line.substr(7, 4), nullptr, 16);
        trace.opcode = std::stoi(opcode_str, nullptr, 16);
        trace.x = std::stoi(line.substr(54, 2), nullptr, 16);
        trace.y = std::stoi(line.substr(61, 2), nullptr, 16);
        trace.a = std::stoi(line.substr(69, 2), nullptr, 16);
        trace.sp = std::stoi(line.substr(78, 2), nullptr, 16);
        trace.pc = std::stoi(line.substr(85, 4), nullptr, 16);
        trace.num_clocks = std::stoi(line.substr(106, 16), nullptr, 16);
        trace.status = std::stoi(line.substr(129, 2), nullptr, 16);

//        printf("%x %s\t\t\t%x %x %x %x %x %llx %x\n", trace.address, trace.disassembled.c_str(), trace.x, trace.y, trace.a, trace.sp, trace.pc, trace.num_clocks, trace.status);
        traces.push_back(trace);
    }

    return traces;
}

struct TestCase {
    std::string name;
    uint16 initial_stack_pointer = 0xfd;
    uint8 initial_status = I | U;
};

TEST_CASE("cpu works", "[6502]") {

    auto test = GENERATE(
            TestCase{"test00-loadstore"},
            TestCase{"test01-andorxor"},
            TestCase{"test02-incdec"},
            TestCase{"test03-bitshifts"},
            TestCase{"test04-jumpsret"},
            TestCase{"test05-reginstrs"},
            TestCase{"test06-addsub", 0xff, 0},
            TestCase{"test07-cmpbeqbne", 0xff, 0});


    auto bus = load_rom("6502-tests/hmc-6502/roms/" + test.name + ".rom");
    bus.write(0xFFFC, 0x00);
    bus.write(0xFFFD, 0xF0);

    auto traces = load_trace("6502-tests/hmc-6502/expectedResults/" + test.name + "-trace.txt");

    SECTION("cpu produces proper disassembly") {
        printf("disassemble %s\n", test.name.c_str());
        auto disassembly = R6502::disassemble(bus, traces.front().address, traces.back().address);
        REQUIRE(disassembly.size() >= traces.size());

        for (auto &trace: traces) {
            auto &disassembled = disassembly[trace.address];
            REQUIRE(disassembled == trace.disassembled);
        }
    }

    SECTION("cpu executes correctly") {
        printf("execute %s\n", test.name.c_str());
        R6502 cpu;
        cpu.reset(bus);
        cpu.sp = test.initial_stack_pointer;
        cpu.status = test.initial_status;

        for (int i = 0; i < 8; i++)
            cpu.clock(bus); // startup sequence

        uint64 clock_time = 0;
        for (auto &trace: traces) {
                printf("executing %04x %-16s (x=%02x y=%02x a=%02x sp=%02x pc=%04x clock=%4llu status=%02x)\n",
                       trace.address, trace.disassembled.c_str(), trace.x, trace.y, trace.a, trace.sp, trace.pc,
                       trace.num_clocks, trace.status);

            clock_time++;
            cpu.clock(bus);
            while (cpu.cycles > 0) {
                clock_time++;
                cpu.clock(bus);
            }

            // we just finished an instruction
            REQUIRE(cpu.last_executed_opcode == trace.opcode);
            REQUIRE(cpu.x == trace.x);
            REQUIRE(cpu.y == trace.y);
            REQUIRE(cpu.a == trace.a);
            REQUIRE(cpu.sp == trace.sp);
            REQUIRE(cpu.pc == trace.pc);
            REQUIRE(clock_time == trace.num_clocks);
            REQUIRE(cpu.status == trace.status);
        }
    }
}
