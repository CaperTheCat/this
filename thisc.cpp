#include <this.hpp>
#include <thisc.hpp>

static const std::set<std::string> builtinGlobals = {"assert"};

uint32_t ThisCompiler::addConstant(const std::variant<int, float, bool, std::string>& val) {
    if (std::holds_alternative<std::string>(val)) {
        const auto& s = std::get<std::string>(val);
        auto it = constMap_.find(s);
        if (it != constMap_.end()) return it->second;
        uint32_t idx = constants_.size();
        constants_.push_back(val);
        constMap_[s] = idx;
        return idx;
    } else if (std::holds_alternative<bool>(val)) {
        bool b = std::get<bool>(val);
        std::string key = b ? "true" : "false";
        auto it = constMap_.find(key);
        if (it != constMap_.end()) return it->second;
        uint32_t idx = constants_.size();
        constants_.push_back(val);
        constMap_[key] = idx;
        return idx;
    } else {
        // numbers: always add (no dedup)
        uint32_t idx = constants_.size();
        constants_.push_back(val);
        return idx;
    }
}

uint32_t ThisCompiler::addFunction(const std::string& name, uint32_t paramCount) {
    uint32_t idx = functions_.size();
    functions_.emplace_back();
    functions_.back().name = name;
    functions_.back().paramCount = paramCount;
    funcNameToIndex_[name] = idx;
    return idx;
}

void ThisCompiler::emitByte(std::vector<uint8_t>& code, uint8_t b) {
    code.push_back(b);
}
void ThisCompiler::emitU32(std::vector<uint8_t>& code, uint32_t v) {
    code.push_back(v & 0xFF);
    code.push_back((v >> 8) & 0xFF);
    code.push_back((v >> 16) & 0xFF);
    code.push_back((v >> 24) & 0xFF);
}
void ThisCompiler::emitOp(std::vector<uint8_t>& code, OpCode op) {
    emitByte(code, static_cast<uint8_t>(op));
}

void ThisCompiler::collectFunctions(AstNode* node) {
    if (auto prog = dynamic_cast<Program*>(node)) {
        std::vector<AstNodePtr> mainBody;
        std::vector<FunctionDef*> userFuncs;
        for (auto& stmt : prog->statements) {
            if (stmt->getType() == AstNodeType::FUNCTION_DEF) {
                userFuncs.push_back(static_cast<FunctionDef*>(stmt.get()));
            } else {
                mainBody.push_back(std::move(stmt));
            }
        }
        // create main function (index 0)
        auto mainFunc = std::make_unique<FunctionDef>("main", std::vector<Parameter>{}, "", std::move(mainBody));
        uint32_t mainIdx = addFunction("main", 0);
        functions_[mainIdx].ast = mainFunc.get();
        syntheticFuncs_.push_back(std::move(mainFunc));
        // now add user-defined functions
        for (auto funcNode : userFuncs) {
            uint32_t idx = addFunction(funcNode->name, funcNode->parameters.size());
            functions_[idx].ast = funcNode;
        }
    }
}

void ThisCompiler::collectLocals(AstNode* node, std::map<std::string, uint32_t>& localMap) {
    switch (node->getType()) {
        case AstNodeType::ASSIGN_STMT: {
            auto assign = static_cast<AssignStmt*>(node);
            if (localMap.find(assign->varName) == localMap.end())
                localMap[assign->varName] = localMap.size();
            collectLocals(assign->value.get(), localMap);
            break;
        }
        case AstNodeType::VARIABLE_EXPR: {
            auto var = static_cast<VariableExpr*>(node);
            if (localMap.find(var->name) == localMap.end())
                localMap[var->name] = localMap.size();
            break;
        }
        case AstNodeType::WHILE_STMT: {
            auto w = static_cast<WhileStmt*>(node);
            collectLocals(w->condition.get(), localMap);
            for (auto& stmt : w->body) collectLocals(stmt.get(), localMap);
            break;
        }
        case AstNodeType::IF_STMT: {
            auto ifs = static_cast<IfStmt*>(node);
            collectLocals(ifs->condition.get(), localMap);
            for (auto& stmt : ifs->body) collectLocals(stmt.get(), localMap);
            for (auto& elif : ifs->elifs) {
                collectLocals(elif.first.get(), localMap);
                for (auto& stmt : elif.second) collectLocals(stmt.get(), localMap);
            }
            for (auto& stmt : ifs->elseBody) collectLocals(stmt.get(), localMap);
            break;
        }
        case AstNodeType::BINARY_EXPR: {
            auto b = static_cast<BinaryExpr*>(node);
            collectLocals(b->left.get(), localMap);
            collectLocals(b->right.get(), localMap);
            break;
        }
        case AstNodeType::CALL_EXPR: {
            auto call = static_cast<CallExpr*>(node);
            collectLocals(call->callee.get(), localMap);
            for (auto& arg : call->arguments) collectLocals(arg.get(), localMap);
            break;
        }
        case AstNodeType::TABLE_EXPR: {
            auto tab = static_cast<TableExpr*>(node);
            for (auto& elem : tab->elements) collectLocals(elem.get(), localMap);
            break;
        }
        default:
            break;
    }
}

void ThisCompiler::generateCode() {
    // first pass: collect locals for each function
    for (auto& func : functions_) {
        for (auto& param : func.ast->parameters) {
            func.localMap[param.name] = func.localMap.size();
        }
        for (auto& stmt : func.ast->body) {
            collectLocals(stmt.get(), func.localMap);
        }
        func.localCount = func.localMap.size();
    }

    // second pass: generate bytecode
    for (auto& func : functions_) {
        generateFunction(func, func.ast);
    }
}

void ThisCompiler::generateStmt(AstNode* node, std::vector<uint8_t>& code, std::map<std::string, uint32_t>& localMap) {
    switch (node->getType()) {
        case AstNodeType::ASSIGN_STMT: {
            auto assign = static_cast<AssignStmt*>(node);
            generateExpr(assign->value.get(), code, localMap);
            uint32_t idx = localMap[assign->varName];
            emitOp(code, OP_STORE_LOCAL);
            emitU32(code, idx);
            break;
        }
        case AstNodeType::WHILE_STMT: {
            auto w = static_cast<WhileStmt*>(node);
            size_t condPos = code.size();
            generateExpr(w->condition.get(), code, localMap);
            emitOp(code, OP_JUMP_IF_FALSE);
            size_t jumpPos = code.size();
            emitU32(code, 0); // placeholder
            for (auto& stmt : w->body) {
                generateStmt(stmt.get(), code, localMap);
            }
            emitOp(code, OP_JUMP);
            emitU32(code, condPos);
            // patch conditional jump
            uint32_t target = code.size();
            for (size_t i = 0; i < 4; ++i) {
                code[jumpPos + i] = (target >> (i*8)) & 0xFF;
            }
            break;
        }
        case AstNodeType::IF_STMT: {
            auto ifs = static_cast<IfStmt*>(node);
            std::vector<size_t> endJumpPositions; // positions of jumps to end
            size_t ifJumpPos = 0;

            // if condition
            generateExpr(ifs->condition.get(), code, localMap);
            emitOp(code, OP_JUMP_IF_FALSE);
            ifJumpPos = code.size();
            emitU32(code, 0); // placeholder
            for (auto& stmt : ifs->body) {
                generateStmt(stmt.get(), code, localMap);
            }
            emitOp(code, OP_JUMP);
            endJumpPositions.push_back(code.size());
            emitU32(code, 0); // to end

            // elifs
            for (auto& elif : ifs->elifs) {
                // patch previous jump to here
                size_t target = code.size();
                for (size_t i = 0; i < 4; ++i) {
                    code[ifJumpPos + i] = (target >> (i*8)) & 0xFF;
                }
                generateExpr(elif.first.get(), code, localMap);
                emitOp(code, OP_JUMP_IF_FALSE);
                ifJumpPos = code.size();
                emitU32(code, 0);
                for (auto& stmt : elif.second) {
                    generateStmt(stmt.get(), code, localMap);
                }
                emitOp(code, OP_JUMP);
                endJumpPositions.push_back(code.size());
                emitU32(code, 0);
            }

            // else
            if (!ifs->elseBody.empty()) {
                // patch last jump to here
                size_t target = code.size();
                for (size_t i = 0; i < 4; ++i) {
                    code[ifJumpPos + i] = (target >> (i*8)) & 0xFF;
                }
                for (auto& stmt : ifs->elseBody) {
                    generateStmt(stmt.get(), code, localMap);
                }
            } else {
                // patch last jump to here
                size_t target = code.size();
                for (size_t i = 0; i < 4; ++i) {
                    code[ifJumpPos + i] = (target >> (i*8)) & 0xFF;
                }
            }

            // patch all end jumps
            size_t endTarget = code.size();
            for (size_t pos : endJumpPositions) {
                for (size_t i = 0; i < 4; ++i) {
                    code[pos + i] = (endTarget >> (i*8)) & 0xFF;
                }
            }
            break;
        }
        case AstNodeType::MODULE_CALL_EXPR: {
            generateExpr(node, code, localMap);
            emitOp(code, OP_POP);   // discard the (nil) return value
            break;
        }
        case AstNodeType::CALL_EXPR: {
            // generate call and discard result
            generateExpr(node, code, localMap);
            emitOp(code, OP_POP);
            break;
        }
        default:
            break;
    }
}

void ThisCompiler::generateExpr(AstNode* node, std::vector<uint8_t>& code, std::map<std::string, uint32_t>& localMap) {
    switch (node->getType()) {
        case AstNodeType::LITERAL_EXPR: {
            auto lit = static_cast<LiteralExpr*>(node);
            uint32_t idx = addConstant(lit->value);
            emitOp(code, OP_LOAD_CONST);
            emitU32(code, idx);
            break;
        }
        case AstNodeType::VARIABLE_EXPR: {
            auto var = static_cast<VariableExpr*>(node);
            auto it = localMap.find(var->name);
            if (it == localMap.end()) {
                std::cerr << "Undefined variable: " << var->name << "\n";
                exit(1);
            }
            uint32_t idx = it->second;
            emitOp(code, OP_LOAD_LOCAL);
            emitU32(code, idx);
            break;
        }
        case AstNodeType::MODULE_CALL_EXPR: {
            auto call = static_cast<ModuleCallExpr*>(node);
            for (auto& arg : call->arguments) {
                generateExpr(arg.get(), code, localMap);
            }
            uint32_t moduleIdx = addConstant(call->moduleName);
            uint32_t funcIdx   = addConstant(call->funcName);
            emitOp(code, OP_CALL_MODULE);
            emitU32(code, moduleIdx);
            emitU32(code, funcIdx);
            emitU32(code, call->arguments.size());
            break;
        }
        case AstNodeType::BINARY_EXPR: {
            auto b = static_cast<BinaryExpr*>(node);
            generateExpr(b->left.get(), code, localMap);
            generateExpr(b->right.get(), code, localMap);
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
                default:
                    std::cerr << "Unknown binary op\n";
                    exit(1);
            }
            emitOp(code, opcode);
            break;
        }
        case AstNodeType::CALL_EXPR: {
            auto call = static_cast<CallExpr*>(node);
            // treat as user-defined function call
            if (call->callee->getType() == AstNodeType::MEMBER_ACCESS_EXPR) {
                auto mem = static_cast<MemberAccessExpr*>(call->callee.get());
                if (mem->object->getType() == AstNodeType::VARIABLE_EXPR) {
                    auto obj = static_cast<VariableExpr*>(mem->object.get());
                    if (builtinModules.count(obj->name)) {
                        std::cerr << "'" << obj->name << "' is undefined. Did you forget to import the standard module " << obj->name << "?\n";
                        exit(1);
                    }
                }
                std::cerr << "Only simple function calls (function names) are supported for user-defined functions.\n";
                exit(1);
            }
            else if (call->callee->getType() == AstNodeType::VARIABLE_EXPR) {
                auto var = static_cast<VariableExpr*>(call->callee.get());
                auto it = funcNameToIndex_.find(var->name);
                if (it == funcNameToIndex_.end()) {
                    if (builtinGlobals.count(var->name)) {
                        for (auto& arg : call->arguments) {
                            generateExpr(arg.get(), code, localMap);
                        }
                        uint32_t nameIdx = addConstant(var->name);
                        emitOp(code, OP_CALL_GLOBAL);
                        emitU32(code, nameIdx);
                        emitU32(code, call->arguments.size());
                        break;
                    }
                    std::cerr << "Undefined function: " << var->name << "\n";
                    exit(1);
                }
                uint32_t funcIdx = it->second;
                for (auto& arg : call->arguments) {
                    generateExpr(arg.get(), code, localMap);
                }
                emitOp(code, OP_CALL);
                emitU32(code, funcIdx);
                emitU32(code, call->arguments.size());
            } else {
                std::cerr << "Only simple function calls supported\n";
                exit(1);
            }
            break;
        }
        case AstNodeType::TABLE_EXPR: {
            auto tab = static_cast<TableExpr*>(node);
            for (auto& elem : tab->elements) {
                generateExpr(elem.get(), code, localMap);
            }
            emitOp(code, OP_MAKE_TABLE);
            emitU32(code, tab->elements.size());
            break;
        }
        default:
            std::cerr << "Unsupported expression type\n";
            exit(1);
    }
}

void ThisCompiler::generateFunction(CompiledFunction& func, FunctionDef* funcNode) {
    // clear the function's code buffer (should be empty already)
    func.code.clear();

    for (auto& stmt : funcNode->body) {
        generateStmt(stmt.get(), func.code, func.localMap);
    }

    // just make sure everything ends with return
    emitOp(func.code, OP_RETURN);
}

void ThisCompiler::writeFile(const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot write output file\n";
        exit(1);
    }
    out.write("THIS", 4);
    uint8_t version = 2;
    out.write(reinterpret_cast<const char*>(&version), 1);

    uint32_t constCount = constants_.size();
    out.write(reinterpret_cast<const char*>(&constCount), 4);
    for (const auto& c : constants_) {
        if (std::holds_alternative<int>(c)) {
            uint8_t type = 0;
            out.write(reinterpret_cast<const char*>(&type), 1);
            int val = std::get<int>(c);
            out.write(reinterpret_cast<const char*>(&val), 4);
        } else if (std::holds_alternative<float>(c)) {
            uint8_t type = 1;
            out.write(reinterpret_cast<const char*>(&type), 1);
            float val = std::get<float>(c);
            out.write(reinterpret_cast<const char*>(&val), 4);
        } else if (std::holds_alternative<std::string>(c)) {
            uint8_t type = 2;
            out.write(reinterpret_cast<const char*>(&type), 1);
            const std::string& s = std::get<std::string>(c);
            uint32_t len = s.size();
            out.write(reinterpret_cast<const char*>(&len), 4);
            out.write(s.data(), len);
        } else if (std::holds_alternative<bool>(c)) {
            uint8_t type = 3;
            out.write(reinterpret_cast<const char*>(&type), 1);
            bool val = std::get<bool>(c);
            out.write(reinterpret_cast<const char*>(&val), 1);
        }
    }

    uint32_t funcCount = functions_.size();
    out.write(reinterpret_cast<const char*>(&funcCount), 4);
    for (const auto& func : functions_) {
        uint32_t paramCount = func.paramCount;
        uint32_t localCount = func.localCount;
        uint32_t codeSize = func.code.size();
        out.write(reinterpret_cast<const char*>(&paramCount), 4);
        out.write(reinterpret_cast<const char*>(&localCount), 4);
        out.write(reinterpret_cast<const char*>(&codeSize), 4);
        out.write(reinterpret_cast<const char*>(func.code.data()), codeSize);
    }
    out.close();
}

void ThisCompiler::compile(const std::string& outputFile) {
    collectFunctions(ast_.get());
    generateCode();
    writeFile(outputFile);
}