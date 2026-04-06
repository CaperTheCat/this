#ifndef TINIT_HPP
#define TINIT_HPP

// inits the vm with the std lib and all that
#include <thisvm.hpp>
#include <tlibio.hpp>
#include <tlibmath.hpp>

inline void initstd(ThisVM& vm) {
    initstdio(vm); // tlibio
    initstdmath(vm); // tlibmath

    vm.builtinGlobals_["assert"] = [](const std::vector<Value>& args) -> Value {
        assert(args.size() == 1); // only one please
        if (!args[0].b) {
            std::cerr << "Assertion failed!\n";
            exit(1);
        }
        return Value(); // nil
    };
}

#endif // TINIT_HPP