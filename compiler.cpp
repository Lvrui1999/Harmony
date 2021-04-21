#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include "util.hpp"

std::map<std::string,int> m1={
    {"ADD",0},{"ADDU",0},{"SUB",0},{"SUBU",0},
    {"MUL",1},{"MULU",2},{"DIV",3},{"AND",4},{"OR",5},
    {"XOR",6},{"NOR",7},{"SLT",8},{"SLTU",9},
    {"SLL",10},{"SRL",11},{"SRA",12},{"JR",13}
},m2 {
    {"ADDI",14},{"ADDIU",15},{"ANDI",16},{"ORI",17},
    {"XORI",18},{"SLLI",19},{"SRLI",20},{"SRAI",21},
    {"LUI",22},{"LW",23},{"SW",24},{"BEQ",25},
    {"BNE",26},{"SLTI",27},{"SLTIU",28}
},m3 {
    {"J",29},{"JAL",30}
},typ1{
        {"ADD",  0},
        {"ADDU", 1},
        {"SUB",  10},
        {"SUBU", 11}
};

unsigned short Gen1(int op,int rs,int rt,int rd,int type) {
    return ((op & ((1 << 5) - 1)) << 11)
           | ((rs & ((1 << 3) - 1)) << 8)
           | ((rt & ((1 << 3) - 1)) << 5)
           | ((rd & ((1 << 3) - 1)) << 2)
           | ((type) & ((1 << 2) - 1));
}

unsigned short Gen2(int op,int rs,int rt,int imm) {
    return ((op & ((1 << 5) - 1)) << 11)
           | ((rs & ((1 << 3) - 1)) << 8)
           | ((rt & ((1 << 3) - 1)) << 5)
           | (imm & ((1 << 5) - 1));
}

unsigned short Gen3(int op,int imm) {
    return ((op & ((1 << 5) - 1)) << 11) | (imm & ((1 << 11) - 1));
}

int ID(std::string x) {
    return std::stoi(x.substr(1, 1));
}

int NUM(std::string x) {
    return std::stoi(x);
}

int main(int argc,char **argv) {
    std::ifstream in(argv[1], std::ios::in);
    std::ofstream out(argv[2], std::ios::out | std::ios::binary);
    std::vector<unsigned short> buf;
    for (std::string s; std::getline(in, s);) {
        std::cerr << s << '\n';
        auto v = Split(s, ' ');
        auto op = v[0];
        auto INS = [&](unsigned short x) {
            buf.push_back(x);
        };
        if (op == "HALT")
            INS(-1);
        else if (op == "PAUSE")
            INS((-1) ^ 1);
        else {
            auto augs = Split(v[1], ',');
            if (op == "MOV") {
                INS(Gen2(m2["ADDI"], ID(augs[1]), 0, NUM(augs[0])));
            } else if (op == "INC" || op == "DEC") {
                INS(Gen2(m2["ADDI"], ID(augs[0]), ID(augs[0]), op == "INC" ? 1 : -1));
            } else if (op == "LUI") {
                INS(Gen2(m2["LUI"], 0, ID(augs[0]), NUM(augs[1])));
            } else if (op == "JR") {
                INS(Gen1(m1["JR"], ID(augs[0]), 0, 0, 0));
            } else if (m1.count(op)) {
                INS(Gen1(m1[op], ID(augs[0]), ID(augs[1]), ID(augs[2]), typ1[op]));
            } else if (m2.count(op)) {
                INS(Gen2(m2[op], ID(augs[0]), ID(augs[1]), NUM(augs[2])));
            } else if (m3.count(op)) {
                INS(Gen3(m3[op], NUM(augs[0])));
            }
        }
    }
    out.write((char *) (&buf[0]), buf.size() << 1);
    out.close();
    return 0;
}