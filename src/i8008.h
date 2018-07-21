#ifndef ALTAIR8800_I8008_H
#define ALTAIR8800_I8008_H

#include <cstdint>
#include <array>

static constexpr int STACK_SIZE = 7;
static constexpr int RAM_SIZE = 16 * 1024; // 16K

struct Registers {
    uint8_t a = 0, b = 0, c = 0, d = 0, e = 0, h = 0, l = 0;
    uint8_t* registerArray[7] = {&a, &b, &c, &d, &e, &h, &l}; // for access from opcodes
    uint16_t pc = 0;
    // technically, the first item on the stack is the program counter and the stack items should be 14 bits long.
    uint16_t stack[STACK_SIZE] = {0, 0, 0, 0, 0, 0, 0};
    int sp = 0;
    // flags aren't a register in the i8008, they're flipflops
    bool sign = false; // result & 0b10000000
    bool zero = false; // result == 0
    bool parity = false; // parity is even
    bool carry = false; // {over,under}flow
    bool* flagArray[4] = {&carry, &zero, &sign, &parity}; // for access from opcodes

    uint16_t getM() {
        return uint16_t(((uint16_t(h) & 0b00111111) << 8) & l); // We only care about the first 6 bits of the H register
    }
};

class Intel8008 {
    public:
        Registers registers;
        std::array<uint8_t, RAM_SIZE> memory;
        explicit Intel8008(std::array<uint8_t, RAM_SIZE>& ram);
        void step();
        void execute(uint8_t opcode);
        uint8_t read();
        void halt();
        void unknownOpcode(uint8_t opcode);
        void push(uint16_t value);
        uint16_t pop();
    private:
        bool halted;
        void updateFlags(uint8_t result);
};

#endif //ALTAIR8800_I8008_H
