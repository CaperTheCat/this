#include <this.hpp>
#include <tparse.hpp>

ThisParser::ThisParser(ThisLexer& lexer) : lexer(lexer) {
    advance(); // load first token
}

static int getPrecedence(ReservedTK tk) {
    switch (tk) {
        case TK_MUL: case TK_DIV: case TK_MOD: return 4;
        case TK_ADD: case TK_SUB:              return 3;
        case TK_LT: case TK_LE: case TK_GT: case TK_GE:
        case TK_EQ: case TK_NE:                 return 2;
        default: return -1;
    }
}

void ThisParser::advance() {
    lexer.thisX_next();
    currentToken = lexer.t;
}

// consumes the current token and advances, or sends an error if not expected with msg
void ThisParser::consume(ReservedTK expected, const std::string& msg) {
    if (currentToken.what != expected) {
        std::ostringstream oss;
        oss << "Line " << lexer.current_line << ": \"" << msg << '\"';
        thisX_error(oss.str());
    }
    advance();
}

bool ThisParser::match(ReservedTK tk) {
    if (currentToken.what == tk) {
        advance();
        return true;
    }
    return false;
}

bool ThisParser::check(ReservedTK tk) {
    return currentToken.what == tk;
}

void ThisParser::thisX_error(const std::string& msg) {
    std::cerr << "Syntax error: " << msg << std::endl;
    // leave
    #undef THIS_GOOD
    exit(1);
}

AstNodePtr ThisParser::thisX_parseProgram() {
    auto program = std::make_unique<Program>();
    while (!check(TK_END)) {
        auto stmt = parseStatement();
        if (stmt) {
            program->statements.push_back(std::move(stmt));
        }
        // continue if nullptr
    }
    return program;
}

AstNodePtr ThisParser::parseWhileStmt() {
    consume(TK_RSV, "Expected 'while'"); // already consumed the 'while' token

    // optional '('
    bool hasParen = match(TK_LPAREN);
    auto cond = parseExpr();
    if (hasParen) consume(TK_RPAREN, "Expected ')' after condition");

    // body must be a block
    consume(TK_LBRACE, "Expected '{' to start while body");
    std::vector<AstNodePtr> body;
    while (!check(TK_RBRACE) && !check(TK_END)) {
        auto stmt = parseStatement();
        if (stmt) body.push_back(std::move(stmt));
        else advance(); // skip (shouldn't happen)
    }
    consume(TK_RBRACE, "Expected '}' after while body");

    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

AstNodePtr ThisParser::parseStatement() {
    if (check(TK_RSV)) {
        std::string word = std::get<std::string>(currentToken.semantics);
        if (word == "import") {
            return parseImportStmt();
        }
        else if (word == "while") {
            return parseWhileStmt();
        }
        else if (word == "def") {
            return parseFunctionDef();
        }
        else {
            thisX_error("Unexpected reserved word: " + word);
        }
    } 
    else if (check(TK_NAME)) {
        // assignment?
        ReservedTK next = lexer.thisX_lookahead();
        if (next == TK_ASSIGN) {
            return parseAssignStmt();
        } else {
            return parseExpr(); // just a ref
        }
    } 
    else {
        return parseExpr();
    }
    return nullptr; // should never happen
}

AstNodePtr ThisParser::parseAssignStmt() {
    std::string varName = std::get<std::string>(currentToken.semantics);
    consume(TK_NAME, "Expected variable name");
    consume(TK_ASSIGN, "Expected '=' in assignment");
    auto expr = parseExpr();
    return std::make_unique<AssignStmt>(varName, std::move(expr));
}

AstNodePtr ThisParser::parsePrimary() {
    if (check(TK_INT)) {
        int val = std::get<int>(currentToken.semantics);
        advance();
        return std::make_unique<LiteralExpr>(val);
    } else if (check(TK_FLT)) {
        float val = std::get<float>(currentToken.semantics);
        advance();
        return std::make_unique<LiteralExpr>(val);
    } else if (check(TK_STR)) {
        std::string val = std::get<std::string>(currentToken.semantics);
        advance();
        return std::make_unique<LiteralExpr>(val);
    } else if (check(TK_NAME)) {
        std::string name = std::get<std::string>(currentToken.semantics);
        advance();
        return std::make_unique<VariableExpr>(name);
    } else if (check(TK_LBRACE)) {
        return parseTable();
    } else if (check(TK_LPAREN)) {
        advance(); // consume '('
        auto expr = parseExpr();
        consume(TK_RPAREN, "Expected ')' after expression");
        return expr;
    } else {
        thisX_error("Unexpected token in primary expression");
        return nullptr;
    }
}

AstNodePtr ThisParser::parsePostfixExpr() {
    auto expr = parsePrimary();

    if (auto varExpr = dynamic_cast<VariableExpr*>(expr.get())) {
        if (importedModules_.find(varExpr->name) != importedModules_.end()) {
            if (match(TK_DOT)) {
                if (!check(TK_NAME)) thisX_error("Expected function name after '.'");
                std::string funcName = std::get<std::string>(currentToken.semantics);
                advance(); // consume function name

                if (!match(TK_LPAREN)) {
                    thisX_error("Expected '(' for module function call");
                }
                std::vector<AstNodePtr> args;
                if (!check(TK_RPAREN)) {
                    args.push_back(parseExpr());
                    while (match(TK_COMMA)) {
                        args.push_back(parseExpr());
                    }
                }
                consume(TK_RPAREN, "Expected ')' after arguments");
                return std::make_unique<ModuleCallExpr>(varExpr->name, funcName, std::move(args));
            }
            // must be a variable?
        }
    }

    while (true) {
        if (match(TK_DOT)) {
            if (!check(TK_NAME)) {
                thisX_error("Expected member name after '.'");
            }
            std::string member = std::get<std::string>(currentToken.semantics);
            advance();
            expr = std::make_unique<MemberAccessExpr>(std::move(expr), member);
        } else if (match(TK_LPAREN)) {
            std::vector<AstNodePtr> args;
            if (!check(TK_RPAREN)) {
                args.push_back(parseExpr());
                while (match(TK_COMMA)) {
                    args.push_back(parseExpr());
                }
            }
            consume(TK_RPAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(std::move(expr), std::move(args));
        } else {
            break;
        }
    }
    return expr;
}

AstNodePtr ThisParser::parseImportStmt() {
    consume(TK_RSV, "Expected 'import'");
    if (!check(TK_NAME)) thisX_error("Expected module name after 'import'");
    std::string moduleName = std::get<std::string>(currentToken.semantics);
    importedModules_.insert(moduleName);
    advance(); // consume module name
    return nullptr;  // no node
}

AstNodePtr ThisParser::parseBinary(int minPrec) {
    auto lhs = parsePostfixExpr();  // primary + postfix (calls, members)

    while (true) {
        ReservedTK op = currentToken.what;
        int prec = getPrecedence(op);
        if (prec < minPrec) break;

        advance(); // consume operator

        auto rhs = parseBinary(prec + 1);

        // map tokens to BinaryExpr::Op
        BinaryExpr::Op binOp;
        switch (op) {
            case TK_ADD: binOp = BinaryExpr::ADD; break;
            case TK_SUB: binOp = BinaryExpr::SUB; break;
            case TK_MUL: binOp = BinaryExpr::MUL; break;
            case TK_DIV: binOp = BinaryExpr::DIV; break;
            case TK_MOD: binOp = BinaryExpr::MOD; break;
            case TK_LT:  binOp = BinaryExpr::LT;  break;
            case TK_LE:  binOp = BinaryExpr::LE;  break;
            case TK_GT:  binOp = BinaryExpr::GT;  break;
            case TK_GE:  binOp = BinaryExpr::GE;  break;
            case TK_EQ:  binOp = BinaryExpr::EQ;  break;
            case TK_NE:  binOp = BinaryExpr::NE;  break;
            default: thisX_error("Unknown binary operator"); return nullptr;
        }
        lhs = std::make_unique<BinaryExpr>(binOp, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

AstNodePtr ThisParser::parseExpr() {
    return parseBinary(0);
}

AstNodePtr ThisParser::parseFunctionDef() {
    consume(TK_RSV, "Expected 'def'");  // eat def
    // func name
    if (!check(TK_NAME)) thisX_error("Expected function name");
    std::string name = std::get<std::string>(currentToken.semantics);
    advance();

    // params
    consume(TK_LPAREN, "Expected '(' after function name");
    std::vector<Parameter> params;
    if (!check(TK_RPAREN)) {
        do {
            if (!check(TK_NAME)) thisX_error("Expected parameter name");
            std::string paramName = std::get<std::string>(currentToken.semantics);
            advance();

            std::string paramType;
            if (match(TK_COLON)) {
                if (!check(TK_NAME)) thisX_error("Expected type name after ':'");
                paramType = std::get<std::string>(currentToken.semantics);
                advance();
            }
            params.push_back({paramName, paramType});
        } while (match(TK_COMMA));
    }
    consume(TK_RPAREN, "Expected ')' after parameters");

    // optional return type (might find use after type update)
    std::string returnType;
    if (match(TK_ARROW)) {
        if (!check(TK_NAME)) thisX_error("Expected return type after '->'");
        returnType = std::get<std::string>(currentToken.semantics);
        advance();
    }

    // body
    consume(TK_LBRACE, "Expected '{' before function body");
    std::vector<AstNodePtr> body;
    while (!check(TK_RBRACE) && !check(TK_END)) {
        auto stmt = parseStatement();
        if (stmt) body.push_back(std::move(stmt));
        else advance();
    }
    consume(TK_RBRACE, "Expected '}' after function body");

    return std::make_unique<FunctionDef>(name, std::move(params),
                                         returnType, std::move(body));
}

AstNodePtr ThisParser::parseTable() {
    consume(TK_LBRACE, "Expected '{' to start table");
    auto table = std::make_unique<TableExpr>();
    if (!check(TK_RBRACE)) {
        table->elements.push_back(parseExpr());
        while (match(TK_COMMA)) {
            table->elements.push_back(parseExpr());
        }
    }
    consume(TK_RBRACE, "Expected '}' after table elements");
    return table;
}

