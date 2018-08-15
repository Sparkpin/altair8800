#include <iostream>
#include <fstream>
#include "monitor.h"
#include "../utils.h"

Monitor::Monitor(Intel8008 cpu) : cpu(cpu) {
}

void Monitor::run() {
    std::string command;
    std::vector<std::string> splitCommand;
    while (isRunning) {
        std::cout << MONITOR_PROMPT;
        std::getline(std::cin, command);
        splitCommand = Altair8008Utils::splitString(command);
        if (splitCommand.empty()) {
            continue;
        } else if (splitCommand[0] == "q" || splitCommand[0] == "quit" || splitCommand[0] == "exit") {
            isRunning = false;
            cpu.halt();
            break;
        } else if (splitCommand[0] == "e" || splitCommand[0] == "examine" || splitCommand[0] == "peek") {
            examine(splitCommand);
        } else if (splitCommand[0] == "dump") {
            dump(splitCommand);
        } else if (splitCommand[0] == "h" || splitCommand[0] == "help") {
            std::string topic = splitCommand.size() > 1 ? splitCommand[1] : "";
            help(topic);
        } else if (splitCommand[0] == "deposit" || splitCommand[0] == "d" || splitCommand[0] == "poke") {
            deposit(splitCommand);
        } else if (splitCommand[0] == "depositnext" || splitCommand[0] == "dn") {
            cpu.registers.pc++;
            deposit(splitCommand);
        } else if (splitCommand[0] == "step" || splitCommand[0] == "s") {
            cpu.step();
        } else if (splitCommand[0] == "load" || splitCommand[0] == "l") {
            load(splitCommand);
        } else if (splitCommand[0] == "pc") {
            std::cout << std::hex << (int)cpu.registers.pc << std::dec << std::endl;
        } else {
            std::cout << "Unknown command. Type \"help\" for a list of valid commands." << std::endl;
        }
    }
}

void Monitor::examine(const std::vector<std::string>& splitCommand) {
    switch (splitCommand.size() - 1) { // amount of arguments
        case 0: {
            printf("%02x", cpu.memory.at(cpu.registers.pc));
            std::cout << std::endl;
            break;
        }
        case 1: {
            int address = std::stoi(splitCommand[1], nullptr, 16);
            if (address >= RAM_SIZE || address < 0) {
                std::cout << "Address out of range. Valid values are 0-" << std::hex << RAM_SIZE - 1 << std::dec << "." << std::endl;
            } else {
                printf("%02x", cpu.memory.at(address));
                std::cout << std::endl;
                cpu.registers.pc = address;
            }
            break;
        }
        case 2: {
            int start = std::stoi(splitCommand[1], nullptr, 16);
            int end = std::stoi(splitCommand[2], nullptr, 16);
            if (start >= RAM_SIZE || start < 0 || end >= RAM_SIZE || end < 0) {
                std::cout << "Addresses out of range (" << start << ", " << end << "). Valid values are 0-" << std::hex << RAM_SIZE - 1 << std::dec << "." << std::endl;
            }
            else if (start > end) {
                std::cout << start << " is greater than " << end << ". Please try examining forwards." << std::endl;
            }
            else {
                for (int address = start; address < end; address += 16) {
                    printf("0x%04x:  ", address);
                    for (int i = 0; i < 16; i++) {
                        printf("%02x ", cpu.memory.at(address + i));
                        if (address + i >= RAM_SIZE || address + i >= end) break;
                        if (i == 7) printf(" ");
                    }
                    std::cout << std::endl;
                }
                cpu.registers.pc = end;
            }
            break;
        }
        default: {
            help("examine");
            break;
        }
    }
}

void Monitor::dump(const std::vector<std::string>& splitCommand) {
    switch (splitCommand.size() - 1) { // amount of arguments
        case 0: {
            for (int address = 0; address < RAM_SIZE; address += 16) {
                printf("0x%04x:  ", address);
                for (int i = 0; i < 16; i++) {
                    printf("%02x ", cpu.memory.at(address + i));
                    if (address + i >= RAM_SIZE) break;
                    if (i == 7) printf(" ");
                }
                std::cout << std::endl;
            }
            break;
        }
        case 1: {
            std::ofstream file(splitCommand[1], std::ios::out|std::ios::trunc|std::ios::binary);
            if (file.is_open()) {
                std::cout << "Writing file..." << std::endl;
                file.write((const char*)cpu.memory.data(), cpu.memory.size());
                file.close();
                std::cout << "Done." << std::endl;
            } else {
                std::cerr << "Couldn't open " << splitCommand[1] << std::endl;
            }
            break;
        }
        default: {
            help("dump");
            break;
        }
    }
}

void Monitor::deposit(const std::vector<std::string>& splitCommand) {
    switch (splitCommand.size() - 1) { // amount of arguments
        case 1: {
            int value = std::stoi(splitCommand[1], nullptr, 16);
            if (value > 0xff || value < 0) {
                std::cout << "Value out of range. Valid values are 0-ff." << std::endl;
            } else {
                cpu.memory[cpu.registers.pc] = value;
            }
            break;
        }
        case 2: {
            int address = std::stoi(splitCommand[1], nullptr, 16);
            int value = std::stoi(splitCommand[2], nullptr, 16);
            if (value > 0xff || value < 0) {
                std::cout << "Value out of range. Valid values are 0-ff." << std::endl;
            } else if (address >= RAM_SIZE || value < 0) {
                std::cout << "Address out of range. Valid adresses are 0-" << std::hex << RAM_SIZE - 1 << std::dec << "." << std::endl;
            } else {
                cpu.memory[address] = value;
            }
            break;
        }
        default: {
            help("deposit");
            break;
        }
    }
}

void Monitor::load(const std::vector<std::string>& splitCommand) {
    switch (splitCommand.size() - 1) { // amount of arguments
        case 1: {
            std::ifstream file(splitCommand[1], std::ios::in|std::ios::binary);
            if (file.is_open()) {
                std::cout << "Reading file..." << std::endl;
                file.read((char *)cpu.memory.data(), cpu.memory.size()); // TODO: should it be loaded from where the program counter is instead?
                file.close();
                std::cout << "Done." << std::endl;
            } else {
                std::cerr << "Couldn't open " << splitCommand[1] << std::endl;
            }
            break;
        }
        default: {
            help("load");
            break;
        }
    }
}

// TODO: find some way to clean this up
void Monitor::help(const std::string& topic) {
    // not the most elegant system but...
    if (topic.empty()) {
        std::cout << "Available commands are: " << std::endl;
        for (const std::string& command : LISTED_COMMANDS) {
            std::cout << command << " ";
        }
        std::cout << std::endl;
        std::cout << "Type help [command] for more help." << std::endl;
        std::cout << "All numerical values are to be entered in hex." << std::endl;
    } else if (topic == "examine" || topic == "e" || topic == "peek") {
        std::cout << "examine (also e, peek)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  examine -- dump memory at program counter" << std::endl;
        std::cout << "  examine [address] -- dump memory at address (and set program counter to that address)" << std::endl;
        std::cout << "  examine [start] [end] -- dump memory between these addresses, inclusive (and set program counter to ending address)" << std::endl;
    } else if (topic == "dump") {
        std::cout << "dump" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  dump -- dump all memory" << std::endl;
        std::cout << "  dump [filename] -- dump memory to file" << std::endl;
    } else if (topic == "help" || topic == "h") {
        std::cout << "help (also h)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  help -- recieve general help and a list of commands" << std::endl;
        std::cout << "  help [command] -- recieve usage information on a command" << std::endl;
        std::cout << "  help help -- break the universe" << std::endl;
    } else if (topic == "quit" || topic == "q" || topic == "exit") {
        std::cout << "quit (also q, exit)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  quit -- close the emulator" << std::endl;
    } else if (topic == "deposit" || topic == "d" || topic == "poke") {
        std::cout << "deposit (also d, poke)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  deposit [value] -- deposit a value at the current address" << std::endl;
        std::cout << "  deposit [address] [value] -- deposit a value at a specific address (does not modify program counter)" << std::endl;
        std::cout << "see also: depositnext" << std::endl;
    } else if (topic == "depositnext" || topic == "dn") {
        std::cout << "depositnext (also dn)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  depositnext [value] -- deposit a value at the next address and increment the program counter to this address (wraps on overflow)" << std::endl;
    } else if (topic == "step" || topic == "s") {
        std::cout << "step (also s)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  step -- run the CPU for one step" << std::endl;
    } else if (topic == "load" || topic == "l") {
        std::cout << "load (also l)" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  load [filename] -- load a file into memory starting at 0x00" << std::endl;
    } else if (topic == "pc") {
        std::cout << "pc" << std::endl;
        std::cout << "USAGE:" << std::endl;
        std::cout << "  pc -- see the current address in the program counter" << std::endl;
    } else {
        std::cout << "No help available for " << topic << std::endl;
    }
}
