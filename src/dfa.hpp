#pragma once
#include <array>
#include "token.hpp"

// every input byte gets bucketed into one of these via classify_char()
// before it ever touches the table -- keeps the table small instead of
// having a row per possible byte value.
enum class CharClass {
    Whitespace, Newline, Digit, Alpha, Quote, Backslash, Slash,
    Dot, Plus, Minus, Star, Percent, Equals, Bang, Lt, Gt, Amp, Pipe,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Colon, Semicolon,
    EndOfInput, Other,
    Count
};

// c is the byte as an unsigned char widened to int, or -1 for EOF
CharClass classify_char(int c);
const char* char_class_name(CharClass c);

// some of these are accepting *and* have outgoing edges at the same time
// (e.g. AfterEquals accepts `=` but still tries to extend into `==`/`=>`) --
// that's what gives us maximal munch for free, see build_table() in dfa.cpp
enum class DState {
    Start,
    InWhitespace,
    AfterSlash, InLineComment,
    InInt,
    InIdent,
    InString, InStringEscape, StringDone,
    AfterDot, DotDotDone,
    AfterMinus, ArrowDone,
    AfterEquals, EqDone, FatArrowDone,
    AfterBang, NotEqDone,
    AfterLt, LtEqDone,
    AfterGt, GtEqDone,
    AfterAmp, AmpAmpDone,
    AfterPipe, PipePipeDone,
    AccLParen, AccRParen, AccLBrace, AccRBrace, AccLBracket, AccRBracket,
    AccComma, AccColon, AccSemi, AccPlus, AccStar, AccPercent,
    AccEof,
    Count
};

const char* dstate_name(DState s);

constexpr int DEAD = -1;

// table[state][class] = next state, or DEAD if there's nowhere to go --
// scanner just falls back to whatever it last accepted
using TransitionTable = std::array<std::array<int, (size_t)CharClass::Count>, (size_t)DState::Count>;
const TransitionTable& dfa_table();

enum class AcceptKind { None, Skip, Token };
struct AcceptInfo {
    AcceptKind kind;
    TokenType token; // meaningful only when kind == Token
};

AcceptInfo accepting_info(DState state);
