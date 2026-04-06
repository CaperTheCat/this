#ifndef TLIB_MATH_HPP
#define TLIB_MATH_HPP

#include <cmath>
#include <thisvm.hpp>

inline void initstdmath(ThisVM& vm) {
    vm.builtinModules_["math"]["sin"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1 || (args[0].type != Value::FLOAT && args[0].type != Value::INT)) {
            std::cerr << "math.sin expects 1 float or int argument\n";
            return Value();
        }
        if (args[0].type == Value::INT) { // promote to float
            return Value(std::sin(static_cast<float>(args[0].i)));
        }
        return Value(std::sin(args[0].f));
    };
    vm.builtinModules_["math"]["cos"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1 || (args[0].type != Value::FLOAT && args[0].type != Value::INT)) {
            std::cerr << "math.cos expects 1 float or int argument\n";
            return Value();
        }
        if (args[0].type == Value::INT) { // promote to float
            return Value(std::cos(static_cast<float>(args[0].i)));
        }
        return Value(std::cos(args[0].f));
    };
    vm.builtinModules_["math"]["tan"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() != 1 || (args[0].type != Value::FLOAT && args[0].type != Value::INT)) {
            std::cerr << "math.tan expects 1 float or int argument\n";
            return Value();
        }
        if (args[0].type == Value::INT) { // promote to float
            return Value(std::tan(static_cast<float>(args[0].i)));
        }
        return Value(std::tan(args[0].f));
    };
}

#endif // TLIB_MATH_HPP