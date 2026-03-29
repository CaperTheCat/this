#ifndef TPARSE_HPP
#define TPARSE_HPP

#include <this.hpp>
#include <tlex.hpp>

enum class AstNodeType {
    PROGRAM,
    IMPORT_STMT,
    ASSIGN_STMT,
    LITERAL_EXPR,
    VARIABLE_EXPR,
    TABLE_EXPR,
    MEMBER_ACCESS_EXPR,
    CALL_EXPR,
    WHILE_STMT,
    BINARY_EXPR,
    FUNCTION_DEF,
    MODULE_CALL_EXPR
};

struct AstNode {
    virtual ~AstNode() = default;
    virtual AstNodeType getType() const = 0;
};

using AstNodePtr = std::unique_ptr<AstNode>;

struct Program : AstNode {
    std::vector<AstNodePtr> statements;
    AstNodeType getType() const override { return AstNodeType::PROGRAM; }
};

struct ImportStmt : AstNode {
    std::string moduleName;
    ImportStmt(const std::string& name) : moduleName(name) {}
    AstNodeType getType() const override { return AstNodeType::IMPORT_STMT; }
};

struct AssignStmt : AstNode {
    std::string varName;
    AstNodePtr value;
    AssignStmt(const std::string& name, AstNodePtr val)
        : varName(name), value(std::move(val)) {}
    AstNodeType getType() const override { return AstNodeType::ASSIGN_STMT; }
};

struct ModuleCallExpr : AstNode {
    std::string moduleName;
    std::string funcName;
    std::vector<AstNodePtr> arguments;
    ModuleCallExpr(const std::string& mod, const std::string& func, std::vector<AstNodePtr> args)
        : moduleName(mod), funcName(func), arguments(std::move(args)) {}
    AstNodeType getType() const override { return AstNodeType::MODULE_CALL_EXPR; }
};

struct LiteralExpr : AstNode {
    std::variant<int, float, std::string> value;
    LiteralExpr(int v) : value(v) {}
    LiteralExpr(float v) : value(v) {}
    LiteralExpr(const std::string& v) : value(v) {}
    AstNodeType getType() const override { return AstNodeType::LITERAL_EXPR; }
};

struct VariableExpr : AstNode {
    std::string name;
    VariableExpr(const std::string& n) : name(n) {}
    AstNodeType getType() const override { return AstNodeType::VARIABLE_EXPR; }
};

struct TableExpr : AstNode {
    std::vector<AstNodePtr> elements;
    AstNodeType getType() const override { return AstNodeType::TABLE_EXPR; }
};

struct Parameter {
    std::string name;
    std::string type; // optional
};

struct FunctionDef : AstNode {
    std::string name;
    std::vector<Parameter> parameters;
    std::string returnType; // optional
    std::vector<AstNodePtr> body;
    // he made a statement so epic even his family cherished him
    FunctionDef(const std::string& n, std::vector<Parameter> params, 
                const std::string& retType, std::vector<AstNodePtr> stmts)
        : name(n), parameters(std::move(params)), returnType(retType), body(std::move(stmts)) {}
    AstNodeType getType() const override { return AstNodeType::FUNCTION_DEF; }
};

struct WhileStmt : AstNode {
    AstNodePtr condition;
    std::vector<AstNodePtr> body;
    WhileStmt(AstNodePtr cond, std::vector<AstNodePtr> stmts)
        : condition(std::move(cond)), body(std::move(stmts)) {}
    AstNodeType getType() const override { return AstNodeType::WHILE_STMT; }
};

struct BinaryExpr : AstNode {
    enum Op { ADD, SUB, MUL, DIV, MOD, LT, LE, GT, GE, EQ, NE } op;
    AstNodePtr left;
    AstNodePtr right;
    BinaryExpr(Op o, AstNodePtr l, AstNodePtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    AstNodeType getType() const override { return AstNodeType::BINARY_EXPR; }
};

struct MemberAccessExpr : AstNode {
    AstNodePtr object;
    std::string member;
    MemberAccessExpr(AstNodePtr obj, const std::string& mem)
        : object(std::move(obj)), member(mem) {}
    AstNodeType getType() const override { return AstNodeType::MEMBER_ACCESS_EXPR; }
};

struct CallExpr : AstNode {
    AstNodePtr callee;
    std::vector<AstNodePtr> arguments;
    CallExpr(AstNodePtr cal, std::vector<AstNodePtr> args)
        : callee(std::move(cal)), arguments(std::move(args)) {}
    AstNodeType getType() const override { return AstNodeType::CALL_EXPR; }
};

class ThisParser {
    THISX_FUNC ThisParser(ThisLexer& lexer);
    THISX_FUNC AstNodePtr thisX_parseProgram();
    THISX_FUNC void thisX_error(const std::string& msg);

    ThisLexer& lexer;
    Token currentToken;
    std::set<std::string> importedModules_;

    THISI_FUNC void advance();
    THISI_FUNC void consume(ReservedTK expected, const std::string& msg);
    THISI_FUNC bool match(ReservedTK tk);
    THISI_FUNC bool check(ReservedTK tk);

    THISI_FUNC AstNodePtr parseStatement();
    THISI_FUNC AstNodePtr parseImportStmt();
    THISI_FUNC AstNodePtr parseWhileStmt();
    THISI_FUNC AstNodePtr parseBinary( int minPrec);
    THISI_FUNC AstNodePtr parseAssignStmt();
    THISI_FUNC AstNodePtr parsePrimary();
    THISI_FUNC AstNodePtr parsePostfixExpr();
    THISI_FUNC AstNodePtr parseExpr();
    THISI_FUNC AstNodePtr parseTable();
    THISI_FUNC AstNodePtr parseFunctionDef();
};

#endif // TPARSE_HPP