#include "dfa.hpp"
#include <initializer_list>

CharClass classify_char(int c) {
    if (c == -1) return CharClass::EndOfInput;
    switch (c) {
        case ' ': case '\t': case '\r': return CharClass::Whitespace;
        case '\n':  return CharClass::Newline;
        case '"':   return CharClass::Quote;
        case '\\':  return CharClass::Backslash;
        case '/':   return CharClass::Slash;
        case '.':   return CharClass::Dot;
        case '+':   return CharClass::Plus;
        case '-':   return CharClass::Minus;
        case '*':   return CharClass::Star;
        case '%':   return CharClass::Percent;
        case '=':   return CharClass::Equals;
        case '!':   return CharClass::Bang;
        case '<':   return CharClass::Lt;
        case '>':   return CharClass::Gt;
        case '&':   return CharClass::Amp;
        case '|':   return CharClass::Pipe;
        case '(':   return CharClass::LParen;
        case ')':   return CharClass::RParen;
        case '{':   return CharClass::LBrace;
        case '}':   return CharClass::RBrace;
        case '[':   return CharClass::LBracket;
        case ']':   return CharClass::RBracket;
        case ',':   return CharClass::Comma;
        case ':':   return CharClass::Colon;
        case ';':   return CharClass::Semicolon;
        default:
            if (c >= '0' && c <= '9') return CharClass::Digit;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') return CharClass::Alpha;
            return CharClass::Other;
    }
}

const char* char_class_name(CharClass c) {
    switch (c) {
        case CharClass::Whitespace: return "WHITESPACE";
        case CharClass::Newline:    return "NEWLINE";
        case CharClass::Digit:      return "DIGIT";
        case CharClass::Alpha:      return "ALPHA_";
        case CharClass::Quote:      return "QUOTE";
        case CharClass::Backslash:  return "BACKSLASH";
        case CharClass::Slash:      return "SLASH";
        case CharClass::Dot:        return "DOT";
        case CharClass::Plus:       return "PLUS";
        case CharClass::Minus:      return "MINUS";
        case CharClass::Star:       return "STAR";
        case CharClass::Percent:    return "PERCENT";
        case CharClass::Equals:     return "EQUALS";
        case CharClass::Bang:       return "BANG";
        case CharClass::Lt:         return "LT";
        case CharClass::Gt:         return "GT";
        case CharClass::Amp:        return "AMP";
        case CharClass::Pipe:       return "PIPE";
        case CharClass::LParen:     return "LPAREN";
        case CharClass::RParen:     return "RPAREN";
        case CharClass::LBrace:     return "LBRACE";
        case CharClass::RBrace:     return "RBRACE";
        case CharClass::LBracket:   return "LBRACKET";
        case CharClass::RBracket:   return "RBRACKET";
        case CharClass::Comma:      return "COMMA";
        case CharClass::Colon:      return "COLON";
        case CharClass::Semicolon:  return "SEMICOLON";
        case CharClass::EndOfInput: return "EOF";
        case CharClass::Other:      return "OTHER";
        default: return "?";
    }
}

const char* dstate_name(DState s) {
    switch (s) {
        case DState::Start:          return "Start";
        case DState::InWhitespace:   return "InWhitespace";
        case DState::AfterSlash:     return "AfterSlash";
        case DState::InLineComment:  return "InLineComment";
        case DState::InInt:          return "InInt";
        case DState::InIdent:        return "InIdent";
        case DState::InString:       return "InString";
        case DState::InStringEscape: return "InStringEscape";
        case DState::StringDone:     return "StringDone";
        case DState::AfterDot:       return "AfterDot";
        case DState::DotDotDone:     return "DotDotDone";
        case DState::AfterMinus:     return "AfterMinus";
        case DState::ArrowDone:      return "ArrowDone";
        case DState::AfterEquals:    return "AfterEquals";
        case DState::EqDone:         return "EqDone";
        case DState::FatArrowDone:   return "FatArrowDone";
        case DState::AfterBang:      return "AfterBang";
        case DState::NotEqDone:      return "NotEqDone";
        case DState::AfterLt:        return "AfterLt";
        case DState::LtEqDone:       return "LtEqDone";
        case DState::AfterGt:        return "AfterGt";
        case DState::GtEqDone:       return "GtEqDone";
        case DState::AfterAmp:       return "AfterAmp";
        case DState::AmpAmpDone:     return "AmpAmpDone";
        case DState::AfterPipe:      return "AfterPipe";
        case DState::PipePipeDone:   return "PipePipeDone";
        case DState::AccLParen:      return "AccLParen";
        case DState::AccRParen:      return "AccRParen";
        case DState::AccLBrace:      return "AccLBrace";
        case DState::AccRBrace:      return "AccRBrace";
        case DState::AccLBracket:    return "AccLBracket";
        case DState::AccRBracket:    return "AccRBracket";
        case DState::AccComma:       return "AccComma";
        case DState::AccColon:       return "AccColon";
        case DState::AccSemi:        return "AccSemi";
        case DState::AccPlus:        return "AccPlus";
        case DState::AccStar:        return "AccStar";
        case DState::AccPercent:     return "AccPercent";
        case DState::AccEof:         return "AccEof";
        default: return "?";
    }
}

namespace {

void set(TransitionTable& t, DState from, CharClass on, DState to) {
    t[(size_t)from][(size_t)on] = (int)to;
}

// sets every class except the ones listed -- for the "eat anything until
// a delimiter shows up" states, string bodies and line comments mostly
void set_all_except(TransitionTable& t, DState from, std::initializer_list<CharClass> excl, DState to) {
    for (size_t c = 0; c < (size_t)CharClass::Count; ++c) {
        bool excluded = false;
        for (auto e : excl) if ((size_t)e == c) excluded = true;
        if (!excluded) t[(size_t)from][c] = (int)to;
    }
}

TransitionTable build_table() {
    TransitionTable t{};
    for (auto& row : t) row.fill(DEAD);

    // Whitespace (space/tab/cr/newline) -- skipped.
    set(t, DState::Start, CharClass::Whitespace, DState::InWhitespace);
    set(t, DState::Start, CharClass::Newline, DState::InWhitespace);
    set(t, DState::InWhitespace, CharClass::Whitespace, DState::InWhitespace);
    set(t, DState::InWhitespace, CharClass::Newline, DState::InWhitespace);

    // `/` and `//` line comments.
    set(t, DState::Start, CharClass::Slash, DState::AfterSlash);
    set(t, DState::AfterSlash, CharClass::Slash, DState::InLineComment);
    set_all_except(t, DState::InLineComment, {CharClass::Newline, CharClass::EndOfInput}, DState::InLineComment);

    // Integer literals.
    set(t, DState::Start, CharClass::Digit, DState::InInt);
    set(t, DState::InInt, CharClass::Digit, DState::InInt);

    // Identifiers / keywords (keyword lookup happens after the DFA accepts
    // an identifier -- see lexer.cpp).
    set(t, DState::Start, CharClass::Alpha, DState::InIdent);
    set(t, DState::InIdent, CharClass::Alpha, DState::InIdent);
    set(t, DState::InIdent, CharClass::Digit, DState::InIdent);

    // strings, with backslash escapes -- a `\` detours through
    // InStringEscape which eats whatever's next no matter what it is
    // (even a quote), so `\"` doesn't end the string early
    set(t, DState::Start, CharClass::Quote, DState::InString);
    set_all_except(t, DState::InString, {CharClass::Quote, CharClass::Backslash, CharClass::EndOfInput}, DState::InString);
    set(t, DState::InString, CharClass::Quote, DState::StringDone);
    set(t, DState::InString, CharClass::Backslash, DState::InStringEscape);
    set_all_except(t, DState::InStringEscape, {CharClass::EndOfInput}, DState::InString);

    // `.` / `..`
    set(t, DState::Start, CharClass::Dot, DState::AfterDot);
    set(t, DState::AfterDot, CharClass::Dot, DState::DotDotDone);

    // `-` / `->`
    set(t, DState::Start, CharClass::Minus, DState::AfterMinus);
    set(t, DState::AfterMinus, CharClass::Gt, DState::ArrowDone);

    // `=` / `==` / `=>`
    set(t, DState::Start, CharClass::Equals, DState::AfterEquals);
    set(t, DState::AfterEquals, CharClass::Equals, DState::EqDone);
    set(t, DState::AfterEquals, CharClass::Gt, DState::FatArrowDone);

    // `!=` only -- bare `!` is not a Nova token (negation is the `not` keyword).
    set(t, DState::Start, CharClass::Bang, DState::AfterBang);
    set(t, DState::AfterBang, CharClass::Equals, DState::NotEqDone);

    // `<` / `<=`
    set(t, DState::Start, CharClass::Lt, DState::AfterLt);
    set(t, DState::AfterLt, CharClass::Equals, DState::LtEqDone);

    // `>` / `>=`
    set(t, DState::Start, CharClass::Gt, DState::AfterGt);
    set(t, DState::AfterGt, CharClass::Equals, DState::GtEqDone);

    // `&` / `&&`
    set(t, DState::Start, CharClass::Amp, DState::AfterAmp);
    set(t, DState::AfterAmp, CharClass::Amp, DState::AmpAmpDone);

    // `|` / `||`
    set(t, DState::Start, CharClass::Pipe, DState::AfterPipe);
    set(t, DState::AfterPipe, CharClass::Pipe, DState::PipePipeDone);

    // Single-character punctuation/operators with no continuation.
    set(t, DState::Start, CharClass::LParen, DState::AccLParen);
    set(t, DState::Start, CharClass::RParen, DState::AccRParen);
    set(t, DState::Start, CharClass::LBrace, DState::AccLBrace);
    set(t, DState::Start, CharClass::RBrace, DState::AccRBrace);
    set(t, DState::Start, CharClass::LBracket, DState::AccLBracket);
    set(t, DState::Start, CharClass::RBracket, DState::AccRBracket);
    set(t, DState::Start, CharClass::Comma, DState::AccComma);
    set(t, DState::Start, CharClass::Colon, DState::AccColon);
    set(t, DState::Start, CharClass::Semicolon, DState::AccSemi);
    set(t, DState::Start, CharClass::Plus, DState::AccPlus);
    set(t, DState::Start, CharClass::Star, DState::AccStar);
    set(t, DState::Start, CharClass::Percent, DState::AccPercent);

    // End of input.
    set(t, DState::Start, CharClass::EndOfInput, DState::AccEof);

    return t;
}

} // namespace

const TransitionTable& dfa_table() {
    static const TransitionTable table = build_table();
    return table;
}

AcceptInfo accepting_info(DState s) {
    switch (s) {
        case DState::InWhitespace:
        case DState::InLineComment:
            return {AcceptKind::Skip, TokenType::Invalid};

        case DState::InInt:        return {AcceptKind::Token, TokenType::IntLit};
        case DState::InIdent:      return {AcceptKind::Token, TokenType::Ident};
        case DState::StringDone:   return {AcceptKind::Token, TokenType::StringLit};

        case DState::AfterSlash:   return {AcceptKind::Token, TokenType::Slash};
        case DState::AfterDot:     return {AcceptKind::Token, TokenType::Dot};
        case DState::DotDotDone:   return {AcceptKind::Token, TokenType::DotDot};
        case DState::AfterMinus:   return {AcceptKind::Token, TokenType::Minus};
        case DState::ArrowDone:    return {AcceptKind::Token, TokenType::Arrow};
        case DState::AfterEquals:  return {AcceptKind::Token, TokenType::Assign};
        case DState::EqDone:       return {AcceptKind::Token, TokenType::Eq};
        case DState::FatArrowDone: return {AcceptKind::Token, TokenType::FatArrow};
        case DState::NotEqDone:    return {AcceptKind::Token, TokenType::NotEq};
        case DState::AfterLt:      return {AcceptKind::Token, TokenType::Lt};
        case DState::LtEqDone:     return {AcceptKind::Token, TokenType::LtEq};
        case DState::AfterGt:      return {AcceptKind::Token, TokenType::Gt};
        case DState::GtEqDone:     return {AcceptKind::Token, TokenType::GtEq};
        case DState::AfterAmp:     return {AcceptKind::Token, TokenType::Amp};
        case DState::AmpAmpDone:   return {AcceptKind::Token, TokenType::AmpAmp};
        case DState::AfterPipe:    return {AcceptKind::Token, TokenType::Pipe};
        case DState::PipePipeDone: return {AcceptKind::Token, TokenType::PipePipe};

        case DState::AccLParen:    return {AcceptKind::Token, TokenType::LParen};
        case DState::AccRParen:    return {AcceptKind::Token, TokenType::RParen};
        case DState::AccLBrace:    return {AcceptKind::Token, TokenType::LBrace};
        case DState::AccRBrace:    return {AcceptKind::Token, TokenType::RBrace};
        case DState::AccLBracket:  return {AcceptKind::Token, TokenType::LBracket};
        case DState::AccRBracket:  return {AcceptKind::Token, TokenType::RBracket};
        case DState::AccComma:     return {AcceptKind::Token, TokenType::Comma};
        case DState::AccColon:     return {AcceptKind::Token, TokenType::Colon};
        case DState::AccSemi:      return {AcceptKind::Token, TokenType::Semicolon};
        case DState::AccPlus:      return {AcceptKind::Token, TokenType::Plus};
        case DState::AccStar:      return {AcceptKind::Token, TokenType::Star};
        case DState::AccPercent:   return {AcceptKind::Token, TokenType::Percent};
        case DState::AccEof:       return {AcceptKind::Token, TokenType::Eof};

        default: return {AcceptKind::None, TokenType::Invalid};
    }
}
