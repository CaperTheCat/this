#include <iostream>
#include <fstream>
#include <vector>
#include <variant>
#include <string>
#include <iomanip>
#include <cstdint>
#include <map>

#include <thisc.hpp>

// stack vals
struct Value {
    enum Type { NIL, INT, FLOAT, STRING, TABLE } type = NIL;
    union {
        int i;
        float f;
    };
    std::string s;
    std::map<std::string, Value> table; // simple table for now

    Value() : type(NIL) {}
    Value(int v) : type(INT), i(v) {}
    Value(float v) : type(FLOAT), f(v) {}
    Value(const std::string& v) : type(STRING), s(v) {}
    Value(std::map<std::string, Value> t) : type(TABLE), table(std::move(t)) {}
};

class ThisVM {
public:
    bool load(const std::string& filename) {
        for (size_t i = 0; i < constants_.size(); ++i) {
            if (constants_[i].type == Value::STRING)
                std::cerr << "string \"" << constants_[i].s << "\" (length " << constants_[i].s.size() << ")\n";
            else if (constants_[i].type == Value::INT)
                std::cerr << "int " << constants_[i].i << "\n";
            else if (constants_[i].type == Value::FLOAT)
                std::cerr << "float " << constants_[i].f << "\n";
        }
        for (uint8_t b : code_) {
            std::cerr << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
        }
        std::ifstream in(filename, std::ios::binary);
        if (!in) return false;

        char magic[4];
        in.read(magic, 4);
        if (std::string(magic,4) != "THIS") return false;

        uint8_t version;
        in.read(reinterpret_cast<char*>(&version), 1);
        if (version != 1) return false;

        uint32_t constCount;
        in.read(reinterpret_cast<char*>(&constCount), 4);
        constants_.resize(constCount);
        for (uint32_t i = 0; i < constCount; ++i) {
            uint8_t type;
            in.read(reinterpret_cast<char*>(&type), 1);
            if (type == 0) { // int
                int val;
                in.read(reinterpret_cast<char*>(&val), 4);
                constants_[i] = Value(val);
            } else if (type == 1) { // float
                float val;
                in.read(reinterpret_cast<char*>(&val), 4);
                constants_[i] = Value(val);
            } else if (type == 2) { // string
                uint32_t len;
                in.read(reinterpret_cast<char*>(&len), 4);
                std::string s(len, '\0');
                in.read(&s[0], len);
                constants_[i] = Value(s);
            }
        }

        uint32_t varCount;
        in.read(reinterpret_cast<char*>(&varCount), 4);
        globals_.resize(varCount);

        uint32_t codeSize;
        in.read(reinterpret_cast<char*>(&codeSize), 4);
        code_.resize(codeSize);
        in.read(reinterpret_cast<char*>(code_.data()), codeSize);

        pc_ = 0;
        return true;
    }

    void run() {
        while (pc_ < code_.size()) {
            OpCode op = static_cast<OpCode>(code_[pc_++]);
            switch (op) {
                case OP_NOP:
                    break;
                case OP_LOAD_CONST: {
                    uint32_t idx = readU32();
                    stack_.push_back(constants_[idx]);
                    break;
                }
                case OP_LOAD_VAR: {
                    uint32_t idx = readU32();
                    stack_.push_back(globals_[idx]);
                    break;
                }
                case OP_STORE_VAR: {
                    uint32_t idx = readU32();
                    globals_[idx] = stack_.back();
                    stack_.pop_back();
                    break;
                }
                case OP_MAKE_TABLE: {
                    uint32_t count = readU32();
                    std::map<std::string, Value> table;
                    // tables are weird don't flame me for it i don't even know if they work that well
                    for (uint32_t i = 0; i < count; ++i) {
                        // elements are pushed in order
                        Value val = stack_.back();
                        stack_.pop_back();
                        table[std::to_string(count - i)] = val;
                    }
                    // push but in reverse
                    stack_.push_back(Value(table));
                    break;
                }
                case OP_CALL_BUILTIN: {
                    uint32_t builtin = readU32();
                    uint32_t argc = readU32();
                    std::vector<Value> args(argc);
                    // arguments are pushed in order, so the first argument is deepest on stack
                    for (uint32_t i = 0; i < argc; ++i) {
                        args[argc - 1 - i] = stack_.back();
                        stack_.pop_back();
                    }
                    if (builtin == BUILTIN_PRINT) {
                        for (size_t i = 0; i < args.size(); ++i) {
                            if (i > 0) std::cout << " ";
                            printValue(args[i]);
                        }
                        stack_.push_back(Value());
                    } else {
                        std::cerr << "Unknown builtin " << builtin << '\n';
                        exit(1);
                    }
                    break;
                }
                case OP_ADD: {
                    Value b = stack_.back(); stack_.pop_back();
                    Value a = stack_.back(); stack_.pop_back();
                    if (a.type == Value::INT && b.type == Value::INT) {
                        stack_.push_back(Value(a.i + b.i));
                    } else if (a.type == Value::FLOAT && b.type == Value::FLOAT) {
                        stack_.push_back(Value(a.f + b.f));
                    } else if (a.type == Value::INT && b.type == Value::FLOAT) {
                        stack_.push_back(Value(static_cast<float>(a.i) + b.f));
                    } else if (a.type == Value::FLOAT && b.type == Value::INT) {
                        stack_.push_back(Value(a.f + static_cast<float>(b.i)));
                    } else {
                        std::cerr << "Type error in addition\n";
                        exit(1);
                    }
                    break;
                }
                case OP_LT: {
                    Value b = stack_.back(); stack_.pop_back();
                    Value a = stack_.back(); stack_.pop_back();
                    bool result = false;
                    // promote to float if needed
                    if (a.type == Value::INT && b.type == Value::INT)
                        result = a.i < b.i;
                    else if (a.type == Value::FLOAT && b.type == Value::FLOAT)
                        result = a.f < b.f;
                    else if (a.type == Value::INT && b.type == Value::FLOAT)
                        result = static_cast<float>(a.i) < b.f;
                    else if (a.type == Value::FLOAT && b.type == Value::INT)
                        result = a.f < static_cast<float>(b.i);
                    else { /* error */ }
                        stack_.push_back(Value(result ? 1 : 0));
                    break;
                }
                case OP_JUMP: {
                    uint32_t target = readU32();
                    pc_ = target; // absolute offset
                    break;
                }
                case OP_JUMP_IF_FALSE: {
                    uint32_t target = readU32();
                    Value cond = stack_.back(); stack_.pop_back();
                    bool isFalse = false;
                    if (cond.type == Value::INT && cond.i == 0) isFalse = true;
                    else if (cond.type == Value::FLOAT && cond.f == 0.0f) isFalse = true;
                    else if (cond.type == Value::NIL) isFalse = true;
                    // truth 
                    if (isFalse) pc_ = target;
                    
                    break;
                }
                case OP_POP: {
                    if (!stack_.empty()) stack_.pop_back();
                    break;
                }
                case OP_HALT:
                    return;
            }
        }
    }

private:
    std::vector<Value> constants_;
    std::vector<Value> globals_;
    std::vector<uint8_t> code_;
    std::vector<Value> stack_;
    size_t pc_ = 0;

    uint32_t readU32() {
        uint32_t v = 0;
        v |= code_[pc_++];
        v |= code_[pc_++] << 8;
        v |= code_[pc_++] << 16;
        v |= code_[pc_++] << 24;
        return v;
    }

    void printValue(const Value& v) {
        switch (v.type) {
            case Value::NIL: std::cout << "nil"; break;
            case Value::INT: std::cout << v.i; break;
            case Value::FLOAT: std::cout << v.f; break;
            case Value::STRING: std::cout << v.s; break;
            case Value::TABLE: std::cout << "{table}"; break;
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tvm <bytecode.tbc>\n";
        return 1;
    }
    ThisVM vm;
    if (!vm.load(argv[1])) {
        std::cerr << "Failed to load bytecode\n";
        return 1;
    }
    vm.run();
    return 0;
}