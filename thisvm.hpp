#ifndef THISVM_HPP
#define THISVM_HPP

#include <this.hpp>

// stack vals
struct Value {
    enum Type { NIL, INT, FLOAT, STRING, TABLE } type = NIL;
    union {
        int i;
        float f;
    };
    std::string s;
    std::map<std::string, Value> table; // simple table for now lol

    Value() : type(NIL) {}
    Value(int v) : type(INT), i(v) {}
    Value(float v) : type(FLOAT), f(v) {}
    Value(const std::string& v) : type(STRING), s(v) {}
    Value(std::map<std::string, Value> t) : type(TABLE), table(std::move(t)) {}
};

struct Function {
    uint32_t paramCount;
    uint32_t localCount;
    std::vector<uint8_t> code;
};

struct Frame {
    std::vector<Value> locals; // size = localCount
    size_t returnPc;
    size_t functionIndex;
};

class ThisVM {

    THISX_FUNC bool thisX_load(const std::string& filename);
    THISX_FUNC void thisX_run();

    ThisVM();

    std::vector<Value> constants_;
    std::vector<Value> globals_;
    std::vector<Function> functions_;
    std::vector<Frame> callStack_;
    std::vector<Value> stack_; // value stack for computations
    std::vector<uint8_t> code_;
    std::map<std::string, std::map<std::string, std::function<Value(const std::vector<Value>&)>>> builtinModules_;
    size_t pc_ = 0; // pc within current func

    size_t currentFuncIndex_;

    THISI_FUNC uint32_t readU32(); // reads from current function's code at pc_
    THISI_FUNC void printValue(const Value& v);
};

#endif // THISVM_HPP