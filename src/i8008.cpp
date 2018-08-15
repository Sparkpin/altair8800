#include <iostream>
#include "i8008.h"

Intel8008::Intel8008(std::array<uint8_t, RAM_SIZE>& ram) : memory(ram) {
    halted = true;
}

uint8_t Intel8008::read() {
    return memory.at(registers.pc & 0x3fff); // cpu can only address 16KiB of RAM
}

// TODO: cycles, clock speed, et al.
void Intel8008::step() {
    execute(read());
}

void Intel8008::execute(uint8_t opcode) {
    registers.pc += 1; // go ahead and move the program counter up for future reads
    switch (opcode & 0b11000000) { // check the first two bits of the opcode to cut down on unnecessary comparisons
        case 0b00 << 6: {
            if (opcode & 0b11111110 == 0) { // for some reason, the lowest bit doesn't matter
                halt(); // HLT - halt
            } else if (opcode & 0b111 == 0b000 && (opcode & 0b00111000) >> 3 != 0b111) { // cannot increment M
                // INr - increment register no carry
                int reg = (opcode & 0b00111000) >> 3;
                *registers.registerArray[reg]++;
                updateFlags(*registers.registerArray[reg]);
            } else if (opcode & 0b111 == 0b001 && (opcode & 0b00111000) >> 3 != 0b111) { // cannot decrement M
                // DCr - decrement register no borrow
                int reg = (opcode & 0b00111000) >> 3;
                *registers.registerArray[reg]--;
                updateFlags(*registers.registerArray[reg]);
            } else {
                switch (opcode & 0b111) {
                    case 0b100: {
                        // Immediate arithmetic
                        uint8_t nextByte = read();
                        registers.pc += 1;
                        switch ((opcode & 0b00111000) >> 3) { // get 3rd, 4th, 5th bit
                            case 0b000:
                                // ADI - add next byte to accumulator, no carry
                                registers.a += nextByte;
                                updateFlags(registers.a);
                                break;
                            case 0b001:
                                // ACI - add next byte and carry to accumulator, no carry
                                registers.a += nextByte + registers.c;
                                updateFlags(registers.a);
                                break;
                            case 0b010:
                                // SUI - subtract next byte, no borrow
                                registers.a -= nextByte;
                                updateFlags(registers.a);
                                break;
                            case 0b011:
                                // SBI - subtract next byte + carry, no borrow
                                registers.a -= nextByte + registers.c;
                                updateFlags(registers.a);
                                break;
                            case 0b100:
                                // NDI - accumulator & next byte
                                registers.a &= nextByte;
                                updateFlags(registers.a);
                                break;
                            case 0b101:
                                // XRI - accumulator xor next byte
                                registers.a ^= nextByte;
                                updateFlags(registers.a);
                                break;
                            case 0b110:
                                // ORI - accumulator or next byte
                                registers.a |= nextByte;
                                updateFlags(registers.a);
                                break;
                            case 0b111:
                                // CPI - subtract next byte from accumulator (don't store), set flags accordingly
                                updateFlags(registers.a - nextByte);
                                break;
                            default:
                                unknownOpcode(opcode);
                                break;
                        } // switch ((opcode & 0b00111000) >> 3)
                        break;
                    }
                    case 0b110: {
                        // Load data immediate
                        uint8_t nextByte = read();
                        registers.pc += 1;
                        if ((opcode & 0b00111000) >> 3 == 0b111) {
                            // LMI - load next byte into RAM address M
                            memory[registers.getM()] = nextByte;
                        } else {
                            // LrI - load next byte into register
                            int reg = (opcode & 0b00111000) >> 3;
                            *registers.registerArray[reg] = nextByte;
                        }
                        break;
                    }
                    case 0b010: {
                        // Bitshifts
                        switch ((opcode & 0b00111000) >> 3) { // get 3rd, 4th, 5th bit
                            case 0b000: {
                                // RLC - rotate accumulator left, first bit (from the left) is shifted into last bit and into carry
                                uint8_t firstBit = (registers.a & 0b10000000) >> 7; // first bit of the accumulator
                                registers.a <<= 1;
                                registers.a |= firstBit; // set the last bit
                                registers.c = firstBit; // put the bit into carry
                                updateFlags(registers.a);
                                break;
                            }
                            case 0b001: {
                                // RRC - rotate accumulator right, last bit (from the left) is shifted into first bit and into carry
                                uint8_t lastBit = registers.a & 1; // last bit of the accumulator
                                registers.a >>= 1;
                                registers.a |= lastBit << 7; // set the first bit
                                registers.c = lastBit; // put the bit into carry
                                updateFlags(registers.a);
                                break;
                            }
                            case 0b010: {
                                // RAL - rotate accumulator left, carry is shifted into last bit and first bit is shifted into carry
                                uint8_t firstBit = (registers.a & 0b10000000) >> 7; // first bit of the accumulator
                                registers.a <<= 1;
                                registers.a |= registers.c & 1; // set the last bit
                                registers.c = firstBit; // put the first bit into carry
                                updateFlags(registers.a);
                                break;
                            }
                            case 0b011: {
                                // RAR - rotate accumulator right, carry is shifted into first bit and last bit is shifted into carry
                                uint8_t lastBit = registers.a & 1; // last bit of the accumulator
                                registers.a >>= 1;
                                registers.a |= (registers.c & 1) << 7; // set the first bit
                                registers.c = lastBit; // put the last bit into carry
                                updateFlags(registers.a);
                                break;
                            }
                            default:
                                unknownOpcode(opcode);
                                break;
                        } // switch ((opcode & 0b00111000) >> 3)
                        break;
                    }
                    case 0b111: {
                        registers.pc = pop(); // RET - return
                        break;
                    }
                    case 0b011: {
                        // conditional return
                        bool condition;
                        condition = *registers.flagArray[opcode &
                                                         0b00011000]; // condition is encoded in the fourth and fifth bytes
                        if (opcode & 0b00100000 == 1) {
                            // RT - return if true
                            if (condition) registers.pc = pop();
                        } else {
                            // RF - return if false
                            if (!condition) registers.pc = pop();
                        }
                        break;
                    }
                    case 0b101: {
                        // RST - restart? more of a subroutine function. push pc to stack, then set pc to opcode & 0b00111000
                        push(registers.pc);
                        registers.pc = opcode & 0b00111000;
                        break;
                    }
                    default:
                        unknownOpcode(opcode);
                        break;
                } // switch(opcode & 0b111)
            } // else
            break;
        } // case 0b00 << 6
        case 0b01 << 6: {
            switch (opcode & 0b111) {
                case 0b100: {
                    // JMP - jump
                    uint8_t low = read();
                    registers.pc++;
                    uint8_t high = read();
                    registers.pc = (high << 8) | low;
                    break;
                }
                case 0b000: {
                    // Jxc - jump if condition
                    bool condition = bool(opcode & 0b00100000 >> 5);
                    bool flag = *registers.flagArray[(opcode & 0b00011000) >> 3];
                    uint8_t low = read();
                    registers.pc++;
                    uint8_t high = read();
                    registers.pc++;
                    if (flag == condition) registers.pc = (high << 8) | low;
                    break;
                }
                case 0b110: {
                    // CAL - call
                    uint8_t low = read();
                    registers.pc++;
                    uint8_t high = read();
                    registers.pc++;
                    push(registers.pc);
                    registers.pc = (high << 8) | low;
                    break;
                }
                case 0b010: {
                    // Cxc - call if condition
                    bool condition = bool(opcode & 0b00100000 >> 5);
                    bool flag = *registers.flagArray[(opcode & 0b00011000) >> 3];
                    uint8_t low = read();
                    registers.pc++;
                    uint8_t high = read();
                    registers.pc++;
                    if (flag == condition) {
                        push(registers.pc);
                        registers.pc = (high << 8) | low;
                    }
                    break;
                }
                default: {
                    if (opcode & 1 == 1) {
                        if (opcode & 0b00110000 == 0) {
                            // INP - read input into accumulator
                            // TODO
                        } else {
                            // OUT - output accumulator contents
                            // TODO
                        }
                    } else {
                        unknownOpcode(opcode);
                    }
                    break;
                }
            }
            break;
        } // case 0b01 << 6
        case 0b10 << 6: {
            // All of these instructions rely on opcode & 0b111 as a register value, which can be 111 for the memory at M.
            // These all set the accumulator, not the register read from (unless that register is the accumulator)
            uint8_t sourceValue;
            if (opcode & 0b111 == 0b111) {
                sourceValue = memory[registers.getM()];
            } else {
                sourceValue = *registers.registerArray[opcode & 0b111];
            }
            uint8_t initialAccumulator = registers.a;
            switch ((opcode & 0b00111000) >> 3) {
                case 0b000: {
                    // ADr - A += sourceValue with carry
                    registers.a += sourceValue;
                    updateFlags(registers.a);
                    registers.carry = registers.a < initialAccumulator;
                    break;
                }
                case 0b001: {
                    // ACr - A += sourceValue + carry with carry
                    registers.a += sourceValue + uint8_t(registers.carry);
                    updateFlags(registers.a);
                    registers.carry = registers.a < initialAccumulator;
                    break;
                }
                case 0b010: {
                    // SUr = A -= sourceValue with borrow
                    /* FIXME:
                     * I picked up the borrow flag solution off good old Wikipedia so it might be wrong.
                     * The 8080 series uses this method, but this CPU uses two's complement subtraction according
                     * to the manual, which matches the 6502 and PowerPC's behaviour instead.
                     */
                    registers.a -= sourceValue;
                    updateFlags(registers.a);
                    registers.carry = initialAccumulator < sourceValue;
                    break;
                }
                case 0b011: {
                    // SUr = A = A - sourceValue - borrow with borrow
                    // FIXME: see above
                    registers.a = registers.a - sourceValue - uint8_t(registers.carry);
                    updateFlags(registers.a);
                    registers.carry = initialAccumulator < sourceValue;
                    break;
                }
                case 0b100 : {
                    // NDr - A &= sourceValue, carry unset
                    registers.a &= sourceValue;
                    updateFlags(registers.a);
                    registers.carry = false;
                    break;
                }
                case 0b101 : {
                    // XRr - A ^= sourceValue, carry unset
                    registers.a ^= sourceValue;
                    updateFlags(registers.a);
                    registers.carry = false;
                    break;
                }
                case 0b110 : {
                    // ORr - A |= sourceValue, carry unset
                    registers.a |= sourceValue;
                    updateFlags(registers.a);
                    registers.carry = false;
                    break;
                }
                case 0b111 : {
                    // CPr - Update flags based on A - sourceValue, A doesn't change, carry unset
                    updateFlags(registers.a - sourceValue);
                    registers.carry = false;
                    break;
                }
                default:
                    unknownOpcode(opcode);
                    break;
            }
            break;
        } // case 0b10 << 6
        case 0b11 << 6: {
            if (opcode == 0xff) {
                halt(); // HLT - halt
            } else if (opcode & 0b111 == 0b111) {
                // LrM - load byte in RAM at address M to register
                *registers.registerArray[opcode & 0b00111000 >> 3] = memory.at(registers.getM());
            } else if ((opcode & 0b00111000) >> 3 == 0b111) {
                // LMr - dump register to memory address M
                memory.at(registers.getM()) = *registers.registerArray[opcode & 0b111];
            } else {
                // Lr1r2 - copy contents of register 2 into register 1
                int src = opcode & 0b111;
                int dest = opcode & 0b00111000 >> 3;
                if (src == dest) break; // copying register to itself, nop
                *registers.registerArray[dest] = *registers.registerArray[src];
            }
            break;
        }
        default: // ?!?!?!??!!?
            unknownOpcode(opcode);
            break;
    }
}

void Intel8008::halt() {
    std::cout << "HALT" << std::endl;
    halted = true;
}

void Intel8008::unknownOpcode(uint8_t opcode) {
    std::cerr << std::hex << "Unknown opcode " << (int)opcode << " at 0x" << registers.pc - 1 << std::dec << std::endl;
}

/**
 * Push a value to the stack
 * @param value Value to be pushed
 */
void Intel8008::push(uint16_t value) {
    if (registers.sp >= STACK_SIZE) {
        std::cerr << "Stack is full, can't push " << value << "!" << std::endl;
        return;
    }
    registers.stack[registers.sp++] = uint16_t(value & 0x3fff); // stack values should be 14 bit
}

/**
 * Pop a value from the stack
 * @return The next value in the stack. If the stack is empty, returns the program counter.
 */
uint16_t Intel8008::pop() {
    if (registers.sp <= 0) {
        std::cerr << "Stack is empty, popping program counter!" << std::endl;
        return registers.pc;
    }
    return registers.stack[--registers.sp];
}

/**
 * Update sign, zero, and parity flags. Does not update carry.
 * @param result The result to use to update the flags
 */
void Intel8008::updateFlags(uint8_t result) {
    registers.sign = (result & 0b10000000) >> 7 == 1;
    registers.zero = result == 0;
    unsigned int numBitsOn = 0;
    // loop through all the bits in the number and tally up every active one
    for (int i = 0; i < 8; i++) {
        numBitsOn += result & 1;
        result >> 1;
    }
    registers.parity = !(numBitsOn & 1); // parity is if the number of enabled bits is even
}
