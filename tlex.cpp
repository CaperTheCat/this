// epic lexer for [this]

#include <this.hpp>
#include <tlex.hpp>

std::array<std::string, 7> RESERVED_WORDS = {"import", "int", "float", "string", "void", "while", "def"};

ThisLexer::ThisLexer(const std::string& source) : source_(source) {
    icurrent = 0;
    current_line = 1;
    last_line = 1;
    look_ahead.what = TK_END;
    current = source_[icurrent];   // first char
}

#ifdef THIS_DEBUG
// reserved to string
std::string thisD_rtstr(ReservedTK token) {
    switch (token) {
    case TK_END:    return "TK_END";
    case TK_INT:    return "TK_INT";
    case TK_FLT:    return "TK_FLT";
    case TK_RSV:    return "TK_RSV";
    case TK_STR:    return "TK_STR";
    case TK_ADD:    return "TK_ADD";
    case TK_NAME:   return "TK_NAME";
    case TK_ASSIGN: return "TK_ASSIGN";
    case TK_DOT:    return "TK_DOT";
    case TK_LPAREN: return "TK_LPAREN";
    case TK_RPAREN: return "TK_RPAREN";
    default:        return "bro what";
    }
}
#endif

// eat and advance
void ThisLexer::nom_nom() {
    icurrent += 1;
    
    if (icurrent < source_.length()) {
        current = source_[icurrent];
    } 
    else {
        current = '\0';
    }
}

// eat and advance a line
void ThisLexer::inc_line() {
    if (current == '\r' && icurrent + 1 < source_.size() && source_[icurrent + 1] == '\n') {
        nom_nom();
        nom_nom();
    } else {
        nom_nom();
    }
    current_line++;
}

bool isreserved(std::string testme) {
    for (std::string word : RESERVED_WORDS) {
        if (testme == word) {
            return true;
        }
    }
    return false;
}

void ThisLexer::read_string(char end) {
    nom_nom(); // skip the opening quote
    buffer.clear();
    while (current != end) {
        if (current == '\0') {
            // unterminated string
            thisX_check(TK_END, "Unterminated string");
            return;
        }
        if (current == '\n' || current == '\r') {
            // unterminated string across lines
            thisX_check(TK_END, "Unterminated string");
            return;
        }
        if (current == '\\') {
            // escape sequence
            nom_nom(); // consume backslash
            switch (current) {
                case 'n':  buffer += '\n'; nom_nom(); break;
                case 't':  buffer += '\t'; nom_nom(); break;
                case '\\': buffer += '\\'; nom_nom(); break;
                case '"':  buffer += '"';  nom_nom(); break;
                case '\'': buffer += '\''; nom_nom(); break;
                default:   buffer += '\\'; buffer += current; nom_nom(); break;
            }
        } else {
            buffer += current;
            nom_nom();
        }
    }
    // skip the closing quote
    nom_nom();
    t.semantics = buffer;
}

ReservedTK ThisLexer::tlex() {
    buffer.clear();
    current = source_[icurrent];
    for (;;) {
        switch(current) {
            case '/':
                nom_nom();
                if (current == '/') { // line comment
                    while (current != '\n' && current != '\r' && current != '\0') {
                        nom_nom();
                    }
                    break;  // continue scanning
                }
                return TK_DIV;   // single then
            case '-':
                nom_nom();
                if (current == '>') { // arrow (->)
                    nom_nom();
                    return TK_ARROW;
                }
                return TK_SUB;
            case '{':
                nom_nom();
                return TK_LBRACE;
            case '}':
                nom_nom();
                return TK_RBRACE;
            case ',':
                nom_nom();
                return TK_COMMA;

            case '*':
                nom_nom();
                return TK_MUL;

            case '%':
                nom_nom();
                return TK_MOD;

            case '<':
                nom_nom();
                if (current == '=') {
                    nom_nom();
                    return TK_LE;
                }
                return TK_LT;

            case '>':
                nom_nom();
                if (current == '=') {
                    nom_nom();
                    return TK_GE;
                }
                return TK_GT;

            case '=':
                nom_nom();
                if (current == '=') {
                    nom_nom();
                    return TK_EQ;
                }
                return TK_ASSIGN;   // single '='

            case '!':
                nom_nom();
                if (current == '=') {
                    nom_nom();
                    return TK_NE;
                }
                thisX_check(TK_END, "Did you mean '!='?"); // no single
                return TK_END;
            // skip this line we're at end
            case '\n': case '\r':
                inc_line();
                break;
            // number
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
                return handle_num();
            case '\0': // end
                return TK_END;
            case '+':
                nom_nom();
                return TK_ADD;
            case ' ':
                while (current == ' ') {
                    nom_nom();
                }
                break;
            case ':':
                nom_nom();
                return TK_COLON;
            case '\"': case '\'':
                read_string(current);
                return TK_STR;
            case '.':
                nom_nom();
                return TK_DOT;
            case '(':
                nom_nom();
                return TK_LPAREN;
            case ')':
                nom_nom();
                return TK_RPAREN;
            case EOF:
                return TK_END;
            default:
                // ids or reserved
                if (isalpha(current) || current == '_') {
                    do {
                        write_buffer(current);
                        nom_nom();
                    } while (isalnum(current) || current == '_');
                    if (isreserved(buffer)) {
                        t.semantics = buffer;
                        return TK_RSV;
                    } else {
                        t.semantics = buffer;
                        return TK_NAME;
                    }
                }
                // end?
                thisX_check(TK_END, "idk ur cooked");
                return TK_END;
        }
    }
    return TK_END;
}

void ThisLexer::write_buffer(int t) {
    buffer += t;
}

ReservedTK ThisLexer::handle_num() {
    char first = current;
    write_buffer(first);
    nom_nom();
    if (first == '0' && check_next('x')) { // hex?
        write_buffer('x'); // force this in
        nom_nom();
    }
    while (isdigit(current)) {               // gather all digits
        write_buffer(current);
        nom_nom();                           // advance to next character
    }
    if (current == '.') { // float?
        write_buffer('.');
        nom_nom(); // go on and assume float
        while (isdigit(current)) {
            write_buffer(current);
            nom_nom();
        }
        t.semantics = std::stof(buffer);            // set semantic value
        return TK_FLT;
    }
    t.semantics = std::stoi(buffer);
    return TK_INT;
}

// move to the next token
void ThisLexer::thisX_next() {
    last_line = current_line;
    if (look_ahead.what != TK_END) { // is there look ahead?
        t.what = look_ahead.what;
        look_ahead.what = TK_END;
    }
    else {
        t.what = tlex();
    }
}

// check and then nom nom if true
bool ThisLexer::check_next(char c) {
    if (current == c) {
        nom_nom();
        return true;
    }
    else return false;
}

// get the lookahead
ReservedTK ThisLexer::thisX_lookahead() {
    assert(look_ahead.what == TK_END);
    look_ahead.what = tlex();
    return look_ahead.what;
}

// check if a token is that and error if no
void ThisLexer::thisX_check(ReservedTK token, const char* msg) {
    if (t.what != token) {
        std::cerr << "Syntax Error: Line " << current_line << ": \"" << msg << '\"';
        // insert gi-hun face here
        #undef THIS_GOOD
        exit(1);
    }
}
