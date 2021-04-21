#ifndef ARKCORE_INSTRUCTIONS_HPP
#define ARKCORE_INSTRUCTIONS_HPP

#include <string>
#include <sstream>
#include <vector>
#include <bitset>
#include "util.hpp"

using namespace std;

unsigned short Extend(int s,int x,int b) {
    std::bitset<16> bit(x);
    for (int i = 15; i >= b; --i)
        bit[i] = bit[b - 1] & s;

    ldebugf("%a to %a\n", x, (unsigned short)bit.to_ullong());

    return (unsigned short) bit.to_ullong();
}

string getReg(int id) {
    static string re[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14",
                          "R15"};
    return re[id];
}

string pack(vector<string> str) {
    string res;
    for (int i = 0; i < str.size() - 1; i++)res += str[i] + ",";
    res += str.back();
    return res;
}


vector<string> compile(unsigned short code) {
    ldebugf("---------------------------\n");
    ldebugf("|compiling %a \n", code);
    ldebugf("---------------------------\n");

    int opt = code & 0xF800;
    opt >>= 11;
    if (opt <= 13) {
        int r1 = (code & 0x0700) >> 8;
        int r2 = (code & 0x00E0) >> 5;
        int r3 = (code & 0x001C) >> 2;
        int tail = code & 0x0003;
        std::map<int, std::string> basic3ops = {
                {0,  "+"},
                {1,  "+"},
                {2,  "-"},
                {3,  "-"},
                {4,  "*"},
                {8,  "*"},
                {12, "/"},
                {16, "&"},
                {20, "|"},
                {24, "^"},
                {28, "~|"},
                {32, "<"},
                {36, "<"},
                {40, "<<"},
                {44, ">>"},
                {48, ">>"}
        };
        int full = opt << 2 | tail;
        if (basic3ops.count(full))
            return {pack({getReg(r2) + "o", "Xi"}), pack({getReg(r3) + "o", "Yi"}),
                    pack({basic3ops[full], "ALUo", getReg(r1) + "i"}), ""};
        else if (opt == 16) {
            return { pack({getReg(r1) + "o", "immi"}), pack({"J"}) };
            //10000 JR ERROR ???
            //return {pack({getReg(r1)+"o","Xi"}),pack({"imm#"+std::to_string(rd),"immo","Yi"}),pack({">>","ALUo",getReg(rt)+"i"}),""};
        }
    } else if (opt <= 28) {
        int r1 = (code & 0x0700) >> 8;
        int r2 = (code & 0x00E0) >> 5;
        int imm = code & 0x001F;
        std::map<int, std::string> basic3ops = {
                {14, "+"},
                {15, "+"},
                {16, "&"},
                {17, "|"},
                {18, "^"},
                {19, "<<"},
                {20, ">>"},
                {21, ">>"},
                {27, "<"},
                {28, "<u"}
        };
        if (basic3ops.count(opt)) {
            return {pack({getReg(r2) + "o", "Xi"}),
                    pack({"imm#" + std::to_string(Extend(basic3ops[opt].back() == 'u', imm, 5)), "immo", "Yi"}),
                    pack({basic3ops[opt], "ALUo", getReg(r1) + "i"}), ""};
        } else {
            std::string imms = std::to_string(Extend(1, imm, 5));
            if (opt == 22) //LUI
                return {pack({getReg(imm) + "o", "Xi"}), pack({"imm#" + imms}), pack({"<<", "ALUo", getReg(r2) + "i"}),
                        ""};
            else if (opt == 23) //LW
                return {pack({getReg(r2) + "o", "Xi"}), pack({"imm#" + imms, "immo", "Yi", "+"}), pack({"ALUo", "ARi"}),
                        pack({"RD", "DRo", getReg(r1) + "i"})};
            else if (opt == 24) //SW
                return {pack({getReg(r2) + "o", "Xi"}), pack({"imm#" + imms, "immo", "Yi", "+"}), pack({"ALUo", "ARi"}),
                        pack({getReg(r1) + "o", "DRi", "WE"})};
            else if (opt == 25) //BEQ
                return {pack({getReg(r1) + "o", "Xi"}), pack({getReg(r2) + "o", "Yi", "-"}),
                        pack({"imm#" + imms, "BEQ"}), ""};
            else if (opt == 26) //BNE
                return {pack({getReg(r1) + "o", "Xi"}), pack({getReg(r2) + "o", "Yi", "-"}),
                        pack({"imm#" + imms, "BNE"}), ""};
        }
    } else {
        auto imms = std::to_string(Extend(0, code & ((1 << 11) - 1), 11));
        if (opt == 29) //J fixed
            return {pack({pack({"imm#" + imms}), pack({"immo", "J"})})};
        else if (opt == 30) //JAL fixed
            return {pack({"PCo", "R", "Xi"}), pack({"imm#" + imms}), pack({"immo", "J"})};
        else if (opt == 31)
            return {code % 2 ? "HALT" : "PAUSE"};
    }
}

#endif //ARKCORE_INSTRUCTIONS_HPP
