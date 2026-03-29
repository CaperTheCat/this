
#include <this.hpp>
#include <thisc.hpp>
#include <thisvm.hpp>

bool ThisVM::thisX_load(const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) return false;

    char magic[4];
    in.read(magic, 4);
    if (std::string(magic,4) != "THIS") return false;

    uint8_t version;
    in.read(reinterpret_cast<char*>(&version), 1);
    if (version != 2) return false;

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

    uint32_t funcCount;
    in.read(reinterpret_cast<char*>(&funcCount), 4);
    functions_.resize(funcCount);
    for (uint32_t i = 0; i < funcCount; ++i) {
        uint32_t paramCount, localCount, codeSize;
        in.read(reinterpret_cast<char*>(&paramCount), 4);
        in.read(reinterpret_cast<char*>(&localCount), 4);
        in.read(reinterpret_cast<char*>(&codeSize), 4);
        functions_[i].paramCount = paramCount;
        functions_[i].localCount = localCount;
        functions_[i].code.resize(codeSize);
        in.read(reinterpret_cast<char*>(functions_[i].code.data()), codeSize);
    }

    // start with the main function (index 0 always)
    currentFuncIndex_ = 0;
    pc_ = 0;
    // frame it
    Frame mainFrame;
    mainFrame.returnPc = 0; // not used at top level
    mainFrame.functionIndex = 0;
    mainFrame.locals.resize(functions_[0].localCount); // all NIL
    callStack_.push_back(mainFrame);

    return true;
}

void ThisVM::thisX_run() {
    while (true) {
        if (pc_ >= functions_[currentFuncIndex_].code.size()) {
            // end of code but no halt?? leave ig
            break;
        }
        OpCode op = static_cast<OpCode>(functions_[currentFuncIndex_].code[pc_++]);
        switch (op) {
            case OP_NOP:
                break;
            case OP_LOAD_CONST: {
                uint32_t idx = readU32();
                stack_.push_back(constants_[idx]);
                break;
            }
            case OP_LOAD_LOCAL: {
                uint32_t idx = readU32();
                stack_.push_back(callStack_.back().locals[idx]);
                break;
            }
            case OP_STORE_LOCAL: {
                uint32_t idx = readU32();
                Value val = stack_.back(); stack_.pop_back();
                callStack_.back().locals[idx] = val;
                break;
            }
            case OP_ADD: {
                Value b = stack_.back(); stack_.pop_back();
                Value a = stack_.back(); stack_.pop_back();
                // type promotion as before
                stack_.push_back(Value(a.i + b.i));
                break;
            }
            case OP_JUMP: {
                uint32_t target = readU32();
                pc_ = target;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint32_t target = readU32();
                Value cond = stack_.back(); stack_.pop_back();
                bool isFalse = false;
                if (cond.type == Value::INT && cond.i == 0) isFalse = true;
                else if (cond.type == Value::FLOAT && cond.f == 0.0f) isFalse = true;
                else if (cond.type == Value::NIL) isFalse = true;
                if (isFalse) pc_ = target;
                break;
            }
            case OP_POP: {
                if (!stack_.empty()) stack_.pop_back();
                break;
            }
            // should do more but im lazy okay
            case OP_CALL: {
                uint32_t funcIdx = readU32();
                uint32_t argCount = readU32();
                // pop arguments in reverse order
                std::vector<Value> args(argCount);
                for (int i = argCount-1; i >= 0; --i) {
                    args[i] = stack_.back(); stack_.pop_back();
                }
                const Function& func = functions_[funcIdx];
                Frame newFrame;
                newFrame.returnPc = pc_;
                newFrame.functionIndex = currentFuncIndex_;
                newFrame.locals.resize(func.localCount);
                // set parameters
                for (uint32_t i = 0; i < argCount && i < func.paramCount; ++i) {
                    newFrame.locals[i] = args[i];
                }
                callStack_.push_back(newFrame);
                // switch to called function
                currentFuncIndex_ = funcIdx;
                pc_ = 0;
                break;
            }
            case OP_CALL_MODULE: {
                uint32_t moduleIdx = readU32();
                uint32_t funcIdx   = readU32();
                uint32_t argCount  = readU32();

                std::vector<Value> args(argCount);
                for (int i = argCount-1; i >= 0; --i) {
                    args[i] = stack_.back();
                    stack_.pop_back();
                }
            
                const std::string& moduleName = constants_[moduleIdx].s;
                const std::string& funcName   = constants_[funcIdx].s;
            
                auto modIt = builtinModules_.find(moduleName);
                if (modIt == builtinModules_.end()) {
                    std::cerr << "Module not found: " << moduleName << "\n";
                    exit(1);
                }
                auto funcIt = modIt->second.find(funcName);
                if (funcIt == modIt->second.end()) {
                    std::cerr << "Function not found in module " << moduleName << ": " << funcName << "\n";
                    exit(1);
                }
            
                Value result = funcIt->second(args);
                stack_.push_back(result);
                break;
            }
            case OP_RETURN: {
                Frame oldFrame = callStack_.back(); callStack_.pop_back();
                if (callStack_.empty()) {
                    // program is done, leave
                    return;
                }
                currentFuncIndex_ = oldFrame.functionIndex;
                pc_ = oldFrame.returnPc;
                break;
            }
            case OP_HALT:
                return;
            default:
                std::cerr << "Unknown opcode " << int(op) << "\n";
                exit(1);
        }
    }
}

uint32_t ThisVM::readU32() {
    const std::vector<uint8_t>& code = functions_[currentFuncIndex_].code;
    uint32_t v = 0;
    v |= code[pc_++];
    v |= code[pc_++] << 8;
    v |= code[pc_++] << 16;
    v |= code[pc_++] << 24;
    return v;
}

void ThisVM::printValue(const Value& v) {
    switch (v.type) {
        case Value::NIL: std::cout << "nil"; break;
        case Value::INT: std::cout << v.i; break;
        case Value::FLOAT: std::cout << v.f; break;
        case Value::STRING: std::cout << v.s; break;
        case Value::TABLE: std::cout << "{table}"; break;
    }
}

ThisVM::ThisVM() {
    // ehh this sucks and needs its own file eventually but whatever
    builtinModules_["io"]["print"] = [](const std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << " ";
            if (args[i].type == Value::STRING) {
                std::cout << args[i].s;
            }
        }
        return Value(); // nil
    };
    builtinModules_["io"]["println"] = [](const std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << " ";
            if (args[i].type == Value::STRING) {
                std::cout << args[i].s;
            }
        }
        std::cout << "\n";
        return Value(); // nil
    };
    builtinModules_["io"]["input"] = []( [[maybe_unused]] const std::vector<Value>& args) -> Value {
        std::string line;
        std::getline(std::cin, line);
        return Value(line);
    };
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: tvm <*.tbc>\n";
        return 1;
    }
    ThisVM vm;
    if (!vm.thisX_load(argv[1])) {
        std::cerr << "Failed to load bytecode\n";
        return 1;
    }
    vm.thisX_run();
    return 0;
}