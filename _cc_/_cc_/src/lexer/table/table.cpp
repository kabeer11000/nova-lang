#include "./interface.h"

CharacterClass XTable::ClassifyCharacter(int c) {
  if (c == -1)
    return CharacterClass::EndOfInput;
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
    return CharacterClass::Whitespace;
  case '\n':
    return CharacterClass::NewLine;
  case '"':
    return CharacterClass::Quote;
  case '\\':
    return CharacterClass::Backslash;
  case '/':
    return CharacterClass::Slash;
  case '.':
    return CharacterClass::Dot;
  case '+':
    return CharacterClass::Plus;
  case '-':
    return CharacterClass::Minus;
  case '*':
    return CharacterClass::Star;
  case '%':
    return CharacterClass::Percent;
  case '=':
    return CharacterClass::Equals;
  case '!':
    return CharacterClass::Bang;
  case '<':
    return CharacterClass::Lt;
  case '>':
    return CharacterClass::Gt;
  case '&':
    return CharacterClass::Amp;
  case '|':
    return CharacterClass::Pipe;
  case '(':
    return CharacterClass::LParen;
  case ')':
    return CharacterClass::RParen;
  case '{':
    return CharacterClass::LBrace;
  case '}':
    return CharacterClass::RBrace;
  case '[':
    return CharacterClass::LBracket;
  case ']':
    return CharacterClass::RBracket;
  case ',':
    return CharacterClass::Comma;
  case ':':
    return CharacterClass::Colon;
  case ';':
    return CharacterClass::SemiColon;
  default:
    if (c >= '0' && c <= '9')
      return CharacterClass::Digit;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
      return CharacterClass::Alpha;
    return CharacterClass::Other;
  }
}
