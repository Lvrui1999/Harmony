#include <iostream>
#include <fstream>
#include <algorithm>
#include "data.hpp"
#include "cpu.hpp"
#include "util.hpp"
#include "memory.hpp"
const int XN = 4e8;
unsigned short buffer[XN];

HardDrive HDD;
Memory mem(&HDD);
DataBusManager bus(&mem);
CPU cpu(&bus);

int main(int argc,char **argv) {
    LogLevel level = LogLevel::warning;
    for (int i = 2; i < argc; i++) {
        static map<string, LogLevel> levOf = {
            {"-ldebug",debug},
            {"-linfo",info},
            {"-lwarning",warning},
            {"-lerror",error},
            {"-lfatal",fatal},
        };
        level = min(level, levOf[string(argv[i])]);
    }

    set_log_mode(level);
    

    linfof("system loading, ver %d...\n", 1);

    std::ifstream in(argv[1], std::ios::in | std::ios::binary);
    // get length of file:
    in.seekg(0, in.end);
    int length = in.tellg();
    in.seekg(0, in.beg);
    in.read((char *) (&buffer[0]), length);
    for (unsigned short i = 0, flag = -1; i < length; ++i) {
        linfof("%a\n", buffer[i]);
        mem.Store((i << 1) + (1 << 15), buffer[i], flag);
    }
    bus.InitPC(1 << 15);

    linfof("Binary file loaded into memory.\n");

    while (true) {
        try {
            if (cpu.halt) {
                lfatalf("CPU halt, exiting...\n");
                break;
            }
            if (cpu.paused) {
                lwarningf("paused. Input register name or memory address to watch its value.\n");
                lwarningf("Format: Rx or ADDR [n -the number of words]\n");
                for (std::string s; (std::getline(std::cin, s), s) != "";) {
                    if (s[0] == 'R')
                        std::cerr << cpu.Reg(s) << '\n';
                    else {
                        unsigned short res, flag = 0;
                        auto v = Split(s, ' ');
                        int n = v.size() == 1 ? 1 : std::stoi(v[1]);
                        for (int i = 0; i < n; ++i)
                            std::cerr << std::dec << (mem.Load(std::stoi(v[0]) + i * 2, flag)) << ' ';
                        std::cerr << std::hex << '\n';
                    }
                }
                cpu.paused = false;
                continue;
            }
            cpu.runStep();
        }
        catch (exception e) {
            lfatalf("%a\n", e.what());
            break;
        }
        //cpu.Show();
    }

    return 0;
}
