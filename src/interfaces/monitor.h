#ifndef ALTAIR8800_MONITOR_H
#define ALTAIR8800_MONITOR_H

#include <string>
#include <vector>
#include "../i8008.h"

constexpr char MONITOR_PROMPT[] = "> ";
const std::string LISTED_COMMANDS[] {"help", "quit", "examine", "deposit", "depositnext", "dump", "step", "load", "pc"};

class Monitor {
    public:
        Intel8008 cpu;
        explicit Monitor(Intel8008 cpu);
        void run();

    private:
        bool isRunning = true;
        void examine(const std::vector<std::string>& splitCommand);
        void dump(const std::vector<std::string>& splitCommand);
        void deposit(const std::vector<std::string>& splitCommand);
        void load(const std::vector<std::string>& splitCommand);
        void help(const std::string& topic);
};


#endif //ALTAIR8800_MONITOR_H
