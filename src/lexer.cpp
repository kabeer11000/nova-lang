#include "lexer.hpp"
#include <stdexcept>
#include <unordered_map>

static const std::unordered_map<std::string, TokenType> keywords = {
    {"fn", TokenType::Fn}, {"let", TokenType::Let}, {"mut", TokenType::Mut},
    {"return", TokenType::Return},
    {"if", TokenType::If}, {"else", TokenType::Else}, {"match", TokenType::Match},
    {"loop", TokenType::Loop}, {"break", TokenType::Break}, {"continue", TokenType::Continue},
    {"region", TokenType::Region}, {"arr", TokenType::Arr}, {"free", TokenType::Free},
    {"extern", TokenType::Extern},
    {"struct", TokenType::Struct}, {"enum", TokenType::Enum},
    {"true", TokenType::True}, {"false", TokenType::False},
    {"and", TokenType::And}, {"or", TokenType::Or}, {"not", TokenType::Not},
    {"Option", TokenType::KwOption}, {"Some", TokenType::KwSome}, {"None", TokenType::KwNone},
    {"Result", TokenType::KwResult}, {"Ok", TokenType::KwOk}, {"Err", TokenType::KwErr},
};

const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::Ident: return "Ident";
        case TokenType::IntLit: return "IntLit";
        case TokenType::StringLit: return "StringLit";
        case TokenType::Fn: return "Fn";
        case TokenType::Let: return "Let";
        case TokenType::Mut: return "Mut";
        case TokenType::Return: return "Return";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::Match: return "Match";
        case TokenType::Loop: return "Loop";
        case TokenType::Break: return "Break";
        case TokenType::Continue: return "Continue";
        case TokenType::Region: return "Region";
        case TokenType::Arr: return "Arr";
        case TokenType::Free: return "Free";
        case TokenType::Extern: return "Extern";
        case TokenType::Struct: return "Struct";
        case TokenType::Enum: return "Enum";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::And: return "And";
        case TokenType::Or: return "Or";
        case TokenType::Not: return "Not";
        case TokenType::KwOption: return "KwOption";
        case TokenType::KwSome: return "KwSome";
        case TokenType::KwNone: return "KwNone";
        case TokenType::KwResult: return "KwResult";
        case TokenType::KwOk: return "KwOk";
        case TokenType::KwErr: return "KwErr";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::LBracket: return "LBracket";
        case TokenType::RBracket: return "RBracket";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Dot: return "Dot";
        case TokenType::DotDot: return "DotDot";
        case TokenType::Arrow: return "Arrow";
        case TokenType::FatArrow: return "FatArrow";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::Assign: return "Assign";
        case TokenType::Eq: return "Eq";
        case TokenType::NotEq: return "NotEq";
        case TokenType::Lt: return "Lt";
        case TokenType::Gt: return "Gt";
        case TokenType::LtEq: return "LtEq";
        case TokenType::GtEq: return "GtEq";
        case TokenType::Amp: return "Amp";
        case TokenType::AmpAmp: return "AmpAmp";
        case TokenType::Pipe: return "Pipe";
        case TokenType::PipePipe: return "PipePipe";
        case TokenType::Eof: return "Eof";
        default: return "Invalid";
    }
}

Lexer::Lexer(std::string source) : src(std::move(source)) {}

// the DFA just figures out which bytes are part of the string, this
// decodes what the escapes actually mean. unknown \X just becomes X.
static std::string unescape_string(const std::string& body) {
    std::string out;
    out.reserve(body.size());
    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) {
            char next = body[++i];
            switch (next) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '0': out += '\0'; break;
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                default: out += next; break;
            }
        } else {
            out += body[i];
        }
    }
    return out;
}

Lexer::ScanResult Lexer::scan_from(size_t start) const {
    const TransitionTable& table = dfa_table();
    DState state = DState::Start;
    size_t cursor = start;

    DState best_state = DState::Count; // sentinel: nothing accepted yet
    size_t best_cursor = start;

    for (;;) {
        int c = (cursor < src.size()) ? static_cast<unsigned char>(src[cursor]) : -1;
        CharClass cls = classify_char(c);
        int next = table[(size_t)state][(size_t)cls];
        if (next == DEAD) break;

        state = static_cast<DState>(next);
        if (c != -1) cursor++;

        AcceptInfo info = accepting_info(state);
        if (info.kind != AcceptKind::None) {
            best_state = state;
            best_cursor = cursor;
        }
        if (c == -1) break; // end of input never loops
    }

    if (best_state == DState::Count) {
        return ScanResult{false, {}, start, start + 1};
    }
    return ScanResult{true, accepting_info(best_state), start, best_cursor};
}

void Lexer::advance_over(const std::string& text) {
    for (char c : text) {
        pos++;
        if (c == '\n') { line++; col = 1; } else { col++; }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    for (;;) {
        int start_line = line, start_col = col;
        size_t start = pos;
        ScanResult result = scan_from(pos);

        if (!result.ok) {
            char bad = start < src.size() ? src[start] : '\0';
            if (bad == '"') {
                throw std::runtime_error("unterminated string literal at line " + std::to_string(start_line));
            }
            throw std::runtime_error("unexpected character '" + std::string(1, bad) + "' at line " + std::to_string(start_line));
        }

        std::string lexeme = src.substr(start, result.end - start);
        advance_over(lexeme);

        if (result.info.kind == AcceptKind::Skip) continue;

        TokenType type = result.info.token;
        if (type == TokenType::StringLit) {
            lexeme = unescape_string(lexeme.substr(1, lexeme.size() - 2)); // strip quotes, then unescape
        } else if (type == TokenType::Ident) {
            auto it = keywords.find(lexeme);
            if (it != keywords.end()) type = it->second;
        }

        tokens.push_back(Token{type, lexeme, start_line, start_col});
        if (type == TokenType::Eof) break;
    }

    return tokens;
}
