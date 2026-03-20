#include <thisc.hpp>

class ThisCompiler {
public:
    ThisCompiler(AstNodePtr ast) : ast_(std::move(ast)) {}

    void compile(const std::string& outputFile) {
        // get stuff from the ast
        collectSymbols(ast_.get());

        // make bytecode
        generate(ast_.get());

        // write that bytecode
        writeFile(outputFile);
    }

private:
    AstNodePtr ast_;
    std::vector<std::variant<int, float, std::string>> constants_;
    std::map<std::string, uint32_t> varIndices_;
    std::vector<uint8_t> code_;
    std::map<std::string, uint32_t> constMap_; // map literal string to index

    uint32_t addConstant(const std::variant<int, float, std::string>& val) {
        if (std::holds_alternative<std::string>(val)) {
            const auto& s = std::get<std::string>(val);
            auto it = constMap_.find(s);
            if (it != constMap_.end()) return it->second;
            uint32_t idx = constants_.size();
            constants_.push_back(val);
            constMap_[s] = idx;
            return idx;
        } else {
            // numbers: always add (no dedup)
            uint32_t idx = constants_.size();
            constants_.push_back(val);
            return idx;
        }
    }

    uint32_t getVarIndex(const std::string& name) {
        auto it = varIndices_.find(name);
        if (it != varIndices_.end()) return it->second;
        uint32_t idx = varIndices_.size();
        varIndices_[name] = idx;
        return idx;
    }

    void emitByte(uint8_t b) {
        code_.push_back(b);
    }

    void emitU32(uint32_t v) {
        code_.push_back(v & 0xFF);
        code_.push_back((v >> 8) & 0xFF);
        code_.push_back((v >> 16) & 0xFF);
        code_.push_back((v >> 24) & 0xFF);
    }

    void emitOp(OpCode op) {
        emitByte(static_cast<uint8_t>(op));
    }

    // first pass
    void collectSymbols(AstNode* node) {
        switch (node->getType()) {
            case AstNodeType::PROGRAM: {
                auto prog = static_cast<Program*>(node);
                for (auto& stmt : prog->statements) {
                    collectSymbols(stmt.get());
                }
                break;
            }
            case AstNodeType::IMPORT_STMT:
                // nothing to collect, shucks
                break;
            case AstNodeType::WHILE_STMT: {
                auto w = static_cast<WhileStmt*>(node);
                collectSymbols(w->condition.get());
                for (auto& stmt : w->body) collectSymbols(stmt.get());
                break;
            }
            case AstNodeType::BINARY_EXPR: {
                auto b = static_cast<BinaryExpr*>(node);
                collectSymbols(b->left.get());
                collectSymbols(b->right.get());
                break;
            }
            case AstNodeType::ASSIGN_STMT: {
                auto assign = static_cast<AssignStmt*>(node);
                getVarIndex(assign->varName);  // ensure variable exists
                collectSymbols(assign->value.get());
                break;
            }
            case AstNodeType::LITERAL_EXPR: {
                auto lit = static_cast<LiteralExpr*>(node);
                addConstant(lit->value);  // register constant
                break;
            }
            case AstNodeType::VARIABLE_EXPR: {
                auto var = static_cast<VariableExpr*>(node);
                getVarIndex(var->name);
                break;
            }
            case AstNodeType::TABLE_EXPR: {
                auto tab = static_cast<TableExpr*>(node);
                for (auto& elem : tab->elements) {
                    collectSymbols(elem.get());
                }
                break;
            }
            case AstNodeType::MEMBER_ACCESS_EXPR: {
                auto mem = static_cast<MemberAccessExpr*>(node);
                collectSymbols(mem->object.get());
                // no registry, it's in here already
                break;
            }
            case AstNodeType::CALL_EXPR: {
                auto call = static_cast<CallExpr*>(node);
                collectSymbols(call->callee.get());
                for (auto& arg : call->arguments) {
                    collectSymbols(arg.get());
                }
                break;
            }
        }
    }

    void generate(AstNode* node) {
        switch (node->getType()) {
            case AstNodeType::PROGRAM: {
                auto prog = static_cast<Program*>(node);
                for (auto& stmt : prog->statements) {
                    generate(stmt.get());
                }
                emitOp(OP_HALT);
                break;
            }
            case AstNodeType::IMPORT_STMT: {
                // don't generate, handled at compile time
                break;
            }
            case AstNodeType::BINARY_EXPR: {
                auto b = static_cast<BinaryExpr*>(node);
                generate(b->left.get());
                generate(b->right.get());
                // map BinaryExpr::Op to OpCode
                OpCode opcode;
                switch (b->op) {
                    case BinaryExpr::ADD: opcode = OP_ADD; break;
                    case BinaryExpr::SUB: opcode = OP_SUB; break;
                    case BinaryExpr::MUL: opcode = OP_MUL; break;
                    case BinaryExpr::DIV: opcode = OP_DIV; break;
                    case BinaryExpr::MOD: opcode = OP_MOD; break;
                    case BinaryExpr::LT:  opcode = OP_LT;  break;
                    case BinaryExpr::LE:  opcode = OP_LE;  break;
                    case BinaryExpr::GT:  opcode = OP_GT;  break;
                    case BinaryExpr::GE:  opcode = OP_GE;  break;
                    case BinaryExpr::EQ:  opcode = OP_EQ;  break;
                    case BinaryExpr::NE:  opcode = OP_NE;  break;
                    default: std::cerr << "Unknown binary op\n"; exit(1);
                }
                emitOp(opcode);
                break;
            }
            case AstNodeType::ASSIGN_STMT: {
                auto assign = static_cast<AssignStmt*>(node);
                generate(assign->value.get());               // value on stack
                uint32_t idx = getVarIndex(assign->varName);
                emitOp(OP_STORE_VAR);
                emitU32(idx);
                break;
            }
            case AstNodeType::WHILE_STMT: {
                auto w = static_cast<WhileStmt*>(node);

                size_t condPos = code_.size();

                generate(w->condition.get());

                emitOp(OP_JUMP_IF_FALSE);
                size_t jumpIfFalsePos = code_.size(); // position of the offset we'll patch
                emitU32(0); // placeholder
                        
                // Generate body
                for (auto& stmt : w->body) {
                    generate(stmt.get());
                }

                emitOp(OP_JUMP);
                emitU32(condPos); // back to condition
            
                size_t afterLoopPos = code_.size();
                uint32_t target = afterLoopPos;  // absolute address
                code_[jumpIfFalsePos]     = target & 0xFF;
                code_[jumpIfFalsePos + 1] = (target >> 8) & 0xFF;
                code_[jumpIfFalsePos + 2] = (target >> 16) & 0xFF;
                code_[jumpIfFalsePos + 3] = (target >> 24) & 0xFF;
            
                break;
            }
            case AstNodeType::LITERAL_EXPR: {
                auto lit = static_cast<LiteralExpr*>(node);
                uint32_t idx = addConstant(lit->value);
                emitOp(OP_LOAD_CONST);
                emitU32(idx);
                break;
            }
            case AstNodeType::VARIABLE_EXPR: {
                auto var = static_cast<VariableExpr*>(node);
                uint32_t idx = getVarIndex(var->name);
                emitOp(OP_LOAD_VAR);
                emitU32(idx);
                break;
            }
            case AstNodeType::TABLE_EXPR: {
                auto tab = static_cast<TableExpr*>(node);
                for (auto& elem : tab->elements) {
                    generate(elem.get());                    // push each element
                }
                emitOp(OP_MAKE_TABLE);
                emitU32(static_cast<uint32_t>(tab->elements.size()));
                break;
            }
            case AstNodeType::MEMBER_ACCESS_EXPR: {
                // would generate code but i need this done asap :sob:
                break;
            }
            case AstNodeType::CALL_EXPR: {
                auto call = static_cast<CallExpr*>(node);
                // Check if callee is a MemberAccessExpr with object "io" and member "print"
                if (call->callee->getType() == AstNodeType::MEMBER_ACCESS_EXPR) {
                    auto mem = static_cast<MemberAccessExpr*>(call->callee.get());
                    if (mem->object->getType() == AstNodeType::VARIABLE_EXPR) {
                        auto obj = static_cast<VariableExpr*>(mem->object.get());
                        if (obj->name == "io" && mem->member == "print") {
                            // built-in print
                            // Generate arguments
                            for (auto& arg : call->arguments) {
                                generate(arg.get());
                            }
                            emitOp(OP_CALL_BUILTIN);
                            emitU32(BUILTIN_PRINT);
                            emitU32(static_cast<uint32_t>(call->arguments.size()));
                            // assume statement
                            emitOp(OP_POP);
                            break;
                        }
                    }
                }
                // this will be better eventually but io.print is all for now
                std::cerr << "Compile error: only io.print() is supported\n";
                exit(1);
            }
        }
    }

    void writeFile(const std::string& filename) {
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            std::cerr << "Cannot write output file\n";
            exit(1);
        }

        // header: magic "THIS", version after
        out.write("THIS", 4);
        uint8_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), 1);

        uint32_t constCount = constants_.size();
        out.write(reinterpret_cast<const char*>(&constCount), 4);

        for (const auto& c : constants_) {
            if (std::holds_alternative<int>(c)) {
                uint8_t type = 0; // int
                out.write(reinterpret_cast<const char*>(&type), 1);
                int val = std::get<int>(c);
                out.write(reinterpret_cast<const char*>(&val), 4);
            } else if (std::holds_alternative<float>(c)) {
                uint8_t type = 1; // float
                out.write(reinterpret_cast<const char*>(&type), 1);
                float val = std::get<float>(c);
                out.write(reinterpret_cast<const char*>(&val), 4);
            } else if (std::holds_alternative<std::string>(c)) {
                uint8_t type = 2; // string
                out.write(reinterpret_cast<const char*>(&type), 1);
                const std::string& s = std::get<std::string>(c);
                uint32_t len = s.size();
                out.write(reinterpret_cast<const char*>(&len), 4);
                out.write(s.data(), len);
            }
        }

        uint32_t varCount = varIndices_.size();
        out.write(reinterpret_cast<const char*>(&varCount), 4);

        uint32_t codeSize = code_.size();
        out.write(reinterpret_cast<const char*>(&codeSize), 4);

        out.write(reinterpret_cast<const char*>(code_.data()), codeSize);

        out.close();
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        // might add support for source.th for convenience yknow
        std::cerr << "Usage: thisc <source.this> [output.tbc]\n";
        return 1;
    }
    std::string sourceFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "out.tbc";

    std::ifstream in(sourceFile);
    if (!in) {
        std::cerr << "Cannot open source file\n";
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    in.close();

    ThisLexer lexer(source);
    ThisParser parser(lexer);
    auto ast = parser.thisX_parseProgram();

    ThisCompiler compiler(std::move(ast));
    compiler.compile(outputFile);

    std::cout << "Compiled to " << outputFile << "\n";
    return 0;
}