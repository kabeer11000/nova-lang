#pragma once
#include <string>

enum class TokenType {
    // literals / identifiers
    Ident, IntLit, StringLit,

    // keywords
    Fn, Let, Mut, Return,
    If, Else, Match,
    Loop, Break, Continue,
    Region, Arr, Free, Extern,
    Struct, Enum,
    True, False,
    And, Or, Not,
    KwOption, KwSome, KwNone,
    KwResult, KwOk, KwErr,

    // punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Colon, Semicolon, Dot, DotDot, Arrow, FatArrow,

    // operators
    Plus, Minus, Star, Slash, Percent,
    Assign, Eq, NotEq, Lt, Gt, LtEq, GtEq,
    Amp, AmpAmp, Pipe, PipePipe,

    Eof,
    Invalid,
};

struct Token {
    TokenType type;
    std::string text;   // raw lexeme (identifier name, literal text, etc.)
    int line;
    int col;
};

const char* token_type_name(TokenType t);
