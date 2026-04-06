#ifndef TLIB_IO_HPP
#define TLIB_IO_HPP

#include <thisvm.hpp>

inline void initstdio(ThisVM& vm) {
    vm.builtinModules_["io"]["print"] = [](const std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].type == Value::STRING) {
                std::cout << args[i].s;
            }
            else if (args[i].type == Value::INT) {
                std::cout << args[i].i;
            }
            else if (args[i].type == Value::FLOAT) {
                std::cout << args[i].f;
            }
            else if (args[i].type == Value::BOOL) {
                std::cout << (args[i].b ? "true" : "false");
            }
        }
        return Value(); // nil
    };
    vm.builtinModules_["io"]["println"] = [](const std::vector<Value>& args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].type == Value::STRING) {
                std::cout << args[i].s;
            }
            else if (args[i].type == Value::INT) {
                std::cout << args[i].i;
            }
            else if (args[i].type == Value::FLOAT) {
                std::cout << args[i].f;
            }
            else if (args[i].type == Value::BOOL) {
                std::cout << (args[i].b ? "true" : "false");
            }
        }
        std::cout << "\n";
        return Value(); // nil
    };
    vm.builtinModules_["io"]["input"] = []( [[maybe_unused]] const std::vector<Value>& args) -> Value {
        std::string line;
        std::getline(std::cin, line);
        return Value(line); // string of input
    };
}

#endif // TLIB_IO_HPP