#ifndef TLEX_HPP
#define TLEX_HPP

#include <this.hpp>

typedef std::variant<double, int, float, std::string> Semantics;

enum ReservedTK {
    // basic 
    TK_INT, TK_END, TK_FLT,
    // string-related (rsv is reserved)
    TK_RSV, TK_STR, TK_NAME,
    // operators
    TK_ASSIGN,
    TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_MOD,
    TK_LT, TK_LE, TK_GT, TK_GE, TK_EQ, TK_NE, TK_AND, TK_OR,
    // single char
    TK_LBRACE, TK_RBRACE, TK_COMMA, 
    TK_DOT, TK_LPAREN, TK_RPAREN, TK_COLON,
    // special stuff
    TK_ARROW /* -> */
};

struct Token {
    ReservedTK what;
    Semantics semantics;
};

class ThisLexer {
    public:

    char current;         // current char
    size_t icurrent;      // current index
    int current_line;     // current line number
    int last_line;        // last line number

    Token t;              // current token
    Token look_ahead;     // next token
    std::string buffer;   // buffer to use for semantics stuff
    std::string source_;  // source code

    ThisLexer(const std::string& source);

    THISI_FUNC void nom_nom();
    THISI_FUNC void inc_line();
    THISI_FUNC ReservedTK handle_num();
    THISI_FUNC void write_buffer(int t);
    THISI_FUNC bool check_next(char c);
    THISI_FUNC void read_string(char end);
    THISI_FUNC ReservedTK tlex();

    THISX_FUNC void       thisX_check(ReservedTK token, const char* msg);
    THISX_FUNC void       thisX_next();
    THISX_FUNC ReservedTK thisX_lookahead();

};

#endif // TLEX_HPP