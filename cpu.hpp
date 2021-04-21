#ifndef ARKCORE_CPU_HPP
#define ARKCORE_CPU_HPP

#include "data.hpp"
#include <map>
#include "util.hpp"
#include <exception>
#include <utility>
#include <functional>
#include <cassert>
#include "instructions.hpp"
#include <iostream>
using namespace std;
//节拍数
const int MAX_STEP=4;

class MicroInstructionException:public exception{
private:
    string e;
public:
    explicit MicroInstructionException(string e):e(std::move(e)){}

    virtual const char* what() {
        return e.c_str();
    }
};


class CPU {
private:
    map<string, StorageUnit> units;
    map<string, function<void()>> microInstructions;
    string stackMicro[MAX_STEP];
    int state = 2;
    int step = 0;
    bool err_overflow, err_divide_0;
    map<string, StorageUnit> interruptions, async_interruptions;
    DataBusManager *bus;

    void init_micros() {
        //ALU micro instructions
        microInstructions["+"] = [&]() {
            units["ALU"].val = units["X"].val + units["Y"].val;
            if (units["ALU"].val > 0 && units["X"].val < 0 && units["Y"].val < 0) {
                err_overflow = true;
            }
            if (units["ALU"].val < 0 && units["X"].val > 0 && units["Y"].val > 0) {
                err_overflow = true;
            }
        };
        microInstructions["-"] = [&]() {
            units["ALU"].val = units["X"].val - units["Y"].val;
        };
        microInstructions["*"] = [&]() {
            units["ALU"].val = units["X"].val * units["Y"].val;
            long long t = (long long) units["X"].val * units["Y"].val;
            if (t != units["ALU"].val)
                err_overflow = true;
        };
        microInstructions["/"] = [&]() {
            if (units["Y"].val == 0) {
                err_divide_0 = true;
                return;
            }
            units["ALU"].val = units["X"].val / units["Y"].val;
        };
        microInstructions["&"] = [&]() {
            units["ALU"].val = units["X"].val & units["Y"].val;
        };
        microInstructions["|"] = [&]() {
            units["ALU"].val = units["X"].val | units["Y"].val;
        };
        microInstructions["^"] = [&]() {
            units["ALU"].val = units["X"].val ^ units["Y"].val;
        };
        microInstructions["~|"] = [&]() {
            units["ALU"].val = ~(units["X"].val | units["Y"].val);
        };
        microInstructions["<"] = [&]() {
            units["ALU"].val = (int) units["X"].val < (int) units["Y"].val;
        };
        microInstructions["<u"] = [&]() {
            units["ALU"].val = units["X"].val < units["Y"].val;
        };
        microInstructions["<<"] = [&]() {
            units["ALU"].val = units["X"].val << units["Y"].val;
        };
        microInstructions[">>"] = [&]() {
            units["ALU"].val = units["X"].val >> units["Y"].val;
        };
        microInstructions[">>u"] = [&]() {
            units["ALU"].val = (int) units["X"].val >> units["Y"].val;
        };
        microInstructions["ADPC"] = [&]() {
            units["PC"].val += 2;
        };
        microInstructions["BEQ"] = [&] {
            if (units["ALU"].val == 0)
                units["PC"].val += units["imm"].val << 1;
        };
        microInstructions["BNE"] = [&] {
            if (units["ALU"].val != 0)
                units["PC"].val += units["imm"].val << 1;
        };
        microInstructions["J"] = [&] {
            units["PC"].val = units["imm"].val << 1;
        };
        /*microInstructions["RD"]=[&](){
            units["DR"].val=0b0000000000101000;
        };
        microInstructions["WE"]=[&](){
            units["DR"].val=0b0000000000101000;
        };*/
        microInstructions["PAUSE"] = [&]() {
            paused = true;
        };
        microInstructions["HALT"] = [&]() {
            halt = true;
        };
    }

    void init_common_regs_and_memory() {
        static string re[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6",
                              "R7"};//,"R8","R9","R10","R11","R12","R13","R14","R15"};
        auto bindIn = [=](StorageUnit *unit) {
            return [=]() {
                unit->sw_in = true;
                bus->update();
            };
        };
        auto bindOut = [=](StorageUnit *unit) {
            return [=]() {
                unit->sw_out = true;
                bus->update();
            };
        };

        for (const auto &s:re) {
            units[s].val = 0;
            bus->regUnit(s, &units[s]);
            bus->addFlow("g", s, FlowDirection::in);
            bus->addFlow("g", s, FlowDirection::out);

            microInstructions[s + "i"] = bindIn(&units[s]);
            microInstructions[s + "o"] = bindOut(&units[s]);
        }


        microInstructions["R0i"] = []() {
            throw MicroInstructionException("R0 can't be overwritten");
        };

        units["MEM"].val = 0;
        bus->regUnit("MEM", &units["MEM"]);
        microInstructions["WE"] = bindIn(&units["MEM"]);
        microInstructions["RD"] = bindOut(&units["MEM"]);
    }

    void init_interruptions() {
        //提供给硬盘的中断异常
        interruptions["IO"].val = 0;
        bus->regUnit("IO", &interruptions["IO"]);
    }

    void reset_flag() {
        err_overflow = false;
        err_divide_0 = false;
    }

public:
    //这两个状态不是cpu本身能够处理的。例如（可能存在的）调试指令将引起cpu暂停，此时可以在main里输出cpu的数据之类的。
    //halt则为cpu停机，此时应该直接退出。
    //异步状态，不会等待完整周期运行完成。
    //中断引起的暂停应该在runStep中处理。
    void Show() {
        for (auto& p : units)
            ldebugf(" |%a %a %a%a\n", p.first, p.second.val, p.second.sw_in, p.second.sw_out);
    }

    unsigned short Reg(std::string s) {
        return units[s].val;
    }

    bool paused = 0, halt = 0;

    explicit CPU(DataBusManager *bus) : bus(bus) {
        auto bindIn = [=](StorageUnit *unit) {
            return [=]() {
                unit->sw_in = 1;
                bus->update();
            };
        };
        auto bindOut = [=](StorageUnit *unit) {
            return [=]() {
                unit->sw_out = 1;
                bus->update();
            };
        };


        for (string k:{"IR", "PC", "DR"}) {
            units[k].val = 0;
            bus->regUnit(k, &units[k]);
            bus->addFlow("g", k, FlowDirection::in);
            bus->addFlow("g", k, FlowDirection::out);

            microInstructions[k + "i"] = bindIn(&units[k]);
            microInstructions[k + "o"] = bindOut(&units[k]);
        }

        units["AR"].val = 0;
        bus->regUnit("AR", &units["AR"]);
        bus->addFlow("g", "AR", FlowDirection::in);
        microInstructions["ARi"] = bindIn(&units["AR"]);

        for (string k:{"X", "Y"}) {
            units[k].val = 0;
            bus->regUnit(k, &units[k]);
            bus->addFlow("g", k, FlowDirection::in);
            microInstructions[k + "i"] = bindIn(&units[k]);
        }

        units["ALU"].val = 0;
        bus->regUnit("ALU", &units["ALU"]);
        bus->addFlow("g", "ALU", FlowDirection::out);
        microInstructions["ALUo"] = bindOut(&units["ALU"]);

        units["imm"].val = 0;
        bus->regUnit("imm", &units["imm"]);
        bus->addFlow("g", "imm", FlowDirection::out);
        microInstructions["immi"] = bindIn(&units["imm"]);
        microInstructions["immo"] = bindOut(&units["imm"]);

        init_micros();
        init_common_regs_and_memory();
        init_interruptions();
    }

    /* StorageUnit *IOSignal() {
        return &interruptions["IO"];
    }*/

    void runPackedMicro(const string &micro) {
        vector<string> micros;
        split(micro, micros, ',');
        for (const auto &i:micros) {
            ldebugf("running %d\n", i);
            //对于立即数指令的特殊处理
            if (i.find("imm#") != i.npos) {
                vector<string> imm;
                split(i, imm, '#');
                int t = 0;
                for (char chr:imm[1])t = t * 10 + chr - '0';
                units["imm"].val = t;
            } else {
                if (!microInstructions.count(i)) {
                    throw MicroInstructionException("No such micro instruction " + i);
                }
                microInstructions[i]();
            }
        }
    }

    void pushMicroInstruction(vector<string> ins) {
        //任何时候，指令的翻译都不应该超过节拍数
        assert(ins.size() <= MAX_STEP);

        int i = 0;
        for (; i < ins.size(); i++) {
            stackMicro[i] = ins[i];
        }
        for (; i < MAX_STEP; i++)stackMicro[i] = "";
    }

    void runStep() {
        bool has = 0;
        for (auto &kv : interruptions) {
            if (kv.second.val) {
                if ((--kv.second.val) == 0) {
                    ldebugf("%a now done!\n", kv.first);
                }
                has = 1;
            }
        }
        if (has) {
            step = (step + 1) % 4;
            return;
        }
        //全局状态更新
        /**
         * 0: 读命令
         * 1: 命令装载，PC++
         * 2: 命令译码
         * 3: 异常处理
         */
        if (!step) {
            (state += 1) %= 3;
            Show();
        }
        ldebugf("step=%a, state=%a\n", step, state);

        if (step == 0) {
            //将第一节拍和译码合在一起
            if (state == 0) {
                ldebugf("reading next instruction\n");
                pushMicroInstruction({"PCo,ARi,RD", "", "", ""});
            } else if (state == 1) {
                ldebugf("load instruction\n");
                pushMicroInstruction({"DRo,IRi,ADPC", "", "", ""});
            } else if (state == 2) {
                ldebugf("compiling instruction to micro instructions\n");
                pushMicroInstruction(compile(units["IR"].val));
            } else if (state == 3) {
                linfof("handling exceptions\n");
                if (err_overflow) {
                    lerrorf("overflow\n");
                }
                if (err_divide_0) {
                    lerrorf("divided by zero\n");
                }
                ldebugf("finish handling exceptions, resetting flags\n");
                reset_flag();
                for (auto &kv:async_interruptions) {
                    if (kv.second.val) {
                        // 这里暂时直接暂停掉CPU执行
                        linfof("paused by external async interruption %d\n", kv.first);
                    }
                }
                (step += 1) %= 4;
                return;
            }
        }

        bus->resetIO();
        runPackedMicro(stackMicro[step]);
        (step += 1) %= 4;
    }
};

#endif //ARKCORE_CPU_HPP