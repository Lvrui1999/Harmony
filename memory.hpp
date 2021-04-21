#ifndef ARKCORE_MEMORY_HPP
#define ARKCORE_MEMORY_HPP
#include <array>
#include <stack>
#include "data.hpp"

const int S_ALL=1<<15,S_MEM=1<<13,S_PAGE=1<<7;
typedef unsigned short Word;
typedef std::array<Word,S_PAGE> Page;
//虚拟地址空间为2*2^16=65536 Words=64 KB
//实际内存大小为2*2^14=16 KB
//页表大小为2*2^8=256 Words

struct HardDrive {
    Page data[S_ALL / S_PAGE];
    std::stack<Word> vacant;

    /*HardDrive() {
        for(int i=0;i<S_ALL/S_PAGE;++i)
            vacant.push(i);
    }*/

    Word Store(Page const &p) {
        Word idx = vacant.top();
        vacant.pop();
        data[idx] = p;
        return idx;
    }

    Page Load(Word pos) {
        vacant.push(pos);
        return data[pos];
    }
};

struct Memory {
    Word loc[S_ALL / S_PAGE];//每一页在硬盘中的位置
    HardDrive *HDD;

    struct {
        unsigned counter;
        Word pageNo;
        Page data;
    } pTable[S_MEM / S_PAGE];

    Memory(HardDrive *HDD) : HDD(HDD) {
        for (int i = 0; i < S_ALL / S_PAGE; ++i)
            loc[i] = i;
        for (int i = 0; i < S_MEM / S_PAGE; ++i)
            pTable[i].counter = -1;
    }

    Word FindPage(Word pageNo, Word &flag) {//LRU
        std::pair<unsigned, Word> mxv = std::make_pair(0, 0);
        bool miss = 1;
        Word id;
        for (int i = 0; i < S_MEM / S_PAGE; ++i) {
            if (pTable[i].pageNo == pageNo) {
                miss = 0;
                id = i;
                pTable[i].counter = 0;
            } else if (pTable[i].counter != -1)
                pTable[i].counter++;
            mxv = std::max(mxv, std::make_pair(pTable[i].counter, (Word) i));
        }
        if (miss) {
            if (flag == 0)
                std::cerr << "Page Missing. Reading from HDD. This may take a while.\n";
            auto &op = pTable[id = mxv.second];
            if (op.counter != -1)
                loc[op.pageNo] = HDD->Store(op.data);
            pTable[mxv.second] = {0, pageNo, HDD->Load(loc[pageNo])};
            //std::cerr<<"Data imported from HDD.\n";
            flag = 8;
        } else
            flag = 4;

        return id;
    }

    void Store(unsigned addr, Word data, Word &flag) {
        if (addr % 2)
            throw "address must be multiple of 2";
        addr >>= 1;
        pTable[FindPage(addr / S_PAGE, flag)].data[addr % S_PAGE] = data;
    }

    Word Load(unsigned addr, Word &flag) {
        if (addr % 2)
            throw "address must be multiple of 2";
        addr >>= 1;
        return pTable[FindPage(addr / S_PAGE, flag)].data[addr % S_PAGE];
    }
};

#endif
