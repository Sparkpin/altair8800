#include <array>
#include <iostream>
#include "i8008.h"

int main() {
    // TODO: run cpu, run interface, push out to serial
    std::array<uint8_t, RAM_SIZE> ram = {};
    Intel8008 cpu(ram);
    cpu.step();
    return 0;
}
