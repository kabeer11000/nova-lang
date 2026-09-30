// Idea here is that we do not make a 256xStates table instead
// build character classes and make a table out of those
// so idea is to check which characters are treated the same way
//
// e.g. Alphabets and Underscores are treated the same e.g.
// _kabeer, __kabeer, kabeer, __ are all valid identifiers no
// special condition exists so we can merge them into a character
// class called Alpha_ for example which matches with both.
//
// Digit for example is another niceity as all are treated the same
// after digit you only expect more
//
// Same way for strings and escaping
//
enum class CharacterClass {
    /* This class tackles space, /r and more */ Whitespace,
    /* This class is seperate from whitespace as it ends // comments */ NewLine,
    Alpha, /* This exists as Start -> InInt exists and Start -> InIndent exists */ Digit,
    Quote, /* Blackslash takes you to Start -> InStringEscape instead of Start -> InString hence it's made a seperate class */ Backslash,
    Slash, Dot, Plus, Minus, Star, Percent, Equals, Bang, Lt, Gt, Amp, Pipe, Comma, Colon, SemiColon, EndOfInput, Other, Count
};
class XTable {
    public:
        // Idea here is we take a signed char, 0-255 -> Characters
        // -1 -> EOF
        CharacterClass ClassifyCharacter(int c);
};
