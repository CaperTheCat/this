#ifndef THISC_HPP
#define THISC_HPP

#include <this.hpp>
#include <tparse.hpp>

static const std::set<std::string> builtinModules = {"io"};

// bytecode ops
enum OpCode : uint8_t {
    OP_NOP,
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_MAKE_TABLE,
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
    OP_CALL_MODULE,
    OP_LOAD_LOCAL,  // u32 local_index
    OP_STORE_LOCAL, // u32 local_index
    OP_CALL, // u32 func_index, u32 arg_count
    OP_RETURN, // pop return val then return
    OP_JUMP,         // unconditional jump (offset)
    OP_JUMP_IF_FALSE,// pop top, jump if false
    OP_HALT
};

struct CompiledFunction {
    std::string name;
    uint32_t paramCount;
    uint32_t localCount;
    std::vector<uint8_t> code;
    std::map<std::string, uint32_t> localMap; // name -> index
    FunctionDef* ast; // pointer to AST node (may be from synthetic or original)
};

class ThisCompiler {
public:
    ThisCompiler(AstNodePtr ast) : ast_(std::move(ast)) {}
    void compile(const std::string& outputFile);

private:
    AstNodePtr ast_;
    std::vector<CompiledFunction> functions_;
    std::map<std::string, uint32_t> funcNameToIndex_;
    std::vector<std::unique_ptr<FunctionDef>> syntheticFuncs_; // hold synthetic main

    // helper to assign a new function entry
    uint32_t addFunction(const std::string& name, uint32_t paramCount);

    // collect functions from AST
    void collectFunctions(AstNode* node);

    // recursively collect local variables inside a function's body
    void collectLocals(AstNode* node, std::map<std::string, uint32_t>& localMap);

    // generate code for all functions
    void generateCode();

    // generate code for a single function
    void generateFunction(CompiledFunction& func, FunctionDef* funcNode);

    // generate statements and expressions into a code buffer
    void generateStmt(AstNode* node, std::vector<uint8_t>& code, std::map<std::string, uint32_t>& localMap);
    void generateExpr(AstNode* node, std::vector<uint8_t>& code, std::map<std::string, uint32_t>& localMap);

    // emit helpers, pass the buffer
    void emitByte(std::vector<uint8_t>& code, uint8_t b);
    void emitU32(std::vector<uint8_t>& code, uint32_t v);
    void emitOp(std::vector<uint8_t>& code, OpCode op);

    // write final bytecode to file
    void writeFile(const std::string& filename);

    // constant table (global)
    std::vector<std::variant<int, float, std::string>> constants_;
    std::map<std::string, uint32_t> constMap_;
    uint32_t addConstant(const std::variant<int, float, std::string>& val);
};

#endif // THISC_HPP