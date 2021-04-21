#ifndef ARKCORE_DATA_HPP
#define ARKCORE_DATA_HPP

#include <map>
#include <utility>
#include <vector>
#include <string>
#include <exception>
#include "util.hpp"
#include "memory.hpp"
using namespace std;

class StorageUnit{
public:
    unsigned short val;
    int sw_in,sw_out;
public:
    StorageUnit(){
        this->val=0;
        sw_in=sw_out=false;
    }

    virtual void toggleIn(bool st){
        sw_in=st;
    }

    virtual void toggleOut(bool st){
        sw_out=st;
    }
};


class StorageException:public exception{
public:
    explicit StorageException(string msg):msg(std::move(msg)){
    }
    virtual const char* what(){
        return msg.c_str();
    }

private:
    string msg;
};


enum FlowDirection{
    in,out
};
using DataFlow=std::pair<std::string,FlowDirection>;

class DataBusManager {
public:
    DataBusManager(Memory *mem) : mem(mem) {}

    void InitPC(unsigned short PC) {
        units["PC"]->val = PC;
    }

    void regUnit(string name, StorageUnit *unit) {
        if (units.count(name)) {
            throw StorageException("double definition for " + name);
        }
        ldebugf("register %d\n", name);
        units[name] = unit;
    }

    ///重置所有存储单元的输入/输出开关
    void resetIO() {
        for (auto kv:units) {
            kv.second->toggleIn(0);
            kv.second->toggleOut(0);
        }
    }

    ///添加数据通路节点
    void addNode(string name) {
        //实际上目前不需要
    }

    void addFlow(string path, string unitId, FlowDirection direction) {
        if (!units.count(unitId)) {
            throw StorageException("undefined source or target");
        }
        ldebugf("add data flow from unit(%d) at (%d) with direction(%d)\n", unitId, path, direction);
        dataFlows[path].emplace_back(unitId, direction);
    }

    void update() {
        auto DoAll = [&]() {
            for (int i = 0; i < dataFlows.size() + 1; i++) {
                for (auto kv:dataFlows) {
                    update_path(kv.second);
                }
            }
        };
        DoAll();
        if (units["MEM"]->sw_out == 1 || units["MEM"]->sw_in == 1) {
            if (units["MEM"]->sw_out == 1) {
                units["MEM"]->sw_out = 2;
                units["DR"]->val = mem->Load(units["AR"]->val, units["IO"]->val);
                ldebugf("Read from %a \n", units["AR"]->val);
                DoAll();
            } else if (units["MEM"]->sw_in == 1) {
                units["MEM"]->sw_in = 2;
                units["IO"]->val = 1;
                mem->Store(units["AR"]->val, units["DR"]->val, units["IO"]->val);
                ldebugf("Store %a to %a \n", units["DR"]->val, units["AR"]->val);

                DoAll();
            }
        }
    }

    StorageUnit *getUnit(string name) {
        if (!units.count(name)) {
            throw StorageException("undefined source or target");
        }
        return units[name];
    }

private:
    std::map<std::string, StorageUnit *> units;
    std::map<string, vector<DataFlow>> dataFlows;
    Memory *mem;

    void update_path(vector<DataFlow> &flows) {
        int ind = 0;
        StorageUnit *src = nullptr;
        for (auto e:flows) {
            StorageUnit *t = getUnit(e.first);
            if (e.second == FlowDirection::out && t->sw_out) {
                ind++;
                src = t;
            }
        }
        if (src == nullptr)return;
        if (ind > 1) {
            throw StorageException("multiple source updating bus");
        }

        for (auto e:flows) {
            StorageUnit *t = getUnit(e.first);
            if (e.second == FlowDirection::in && t->sw_in) {
                t->val = src->val;
            }
        }
    }
};

#endif //ARKCORE_DATA_HPP
