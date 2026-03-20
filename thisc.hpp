#ifndef THISC_HPP
#define THISC_HPP

#include "tparse.hpp"
#include <fstream>
#include <vector>
#include <map>
#include <variant>
#include <cstdint>

// bytecode ops
enum OpCode : uint8_t {
    OP_NOP,
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_MAKE_TABLE,
    OP_CALL_BUILTIN,
    OP_POP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_LT, 
    OP_LE, 
    OP_GT, 
    OP_GE, 
    OP_EQ, 
    OP_NE, 
    OP_JUMP,         // unconditional jump (offset)
    OP_JUMP_IF_FALSE,// pop top, jump if false
    OP_HALT
};

// might only be a temporary thing
enum BuiltinID : uint32_t {
    BUILTIN_PRINT = 0
};

// too lazy to do anything else

#endif