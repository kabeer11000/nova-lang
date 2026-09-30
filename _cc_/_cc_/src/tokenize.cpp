#include "./logger.h"
#include "./stream.h"
#include <string>
#include <vector>

enum TokenType {
  // Literals and Identifiers
  Ident,
  IntLit,
  StringLit,

  // Functions, Variables and Control flow
  Fn,
  Let,
  Mut,
  Return,
  If,
  Else,
  Match,
  Loop,
  Break,
  Continue,
  Region,
  Arr,
  Free,
  Extern,
  Struct,
  Enum,
  True,
  False,
  Or,
  Not,
  Option,
  Some,
  None,
  Result,
  Ok,
  Err,

  // Punctuation
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Colon,
  SemiColon,
  Dot,
  DotDot,
  Arrow,
  FatArrow,

  // Operators
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Assign,
  Eq,
  NotEq,
  Lt,
  Gt,
  LtEq,
  GtEq,
  /* Ampersand */ Amp,
  /* And */ AmpAmp,
  /* Bitwise */ Pipe,
  PipePipe
};
struct TokenLoc {
  int line;
  int col;
};
struct Token {
  TokenType type;     // class-part
  std::string lexeme; // value-part
  TokenLoc location;
};

class Tokenizer {
public:
  Stream &stream;
  Tokenizer(Stream &s) : stream(s) {};
  std::vector<Token> tokenize() {
    while (!stream.is_eof()) {
        
    }
  };
};
