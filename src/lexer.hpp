#pragma once
#include <string>
#include <vector>
#include "token.hpp"
#include "dfa.hpp"

// walks the transition table from dfa.cpp to tokenize a source string.
// throws std::runtime_error on unterminated strings / bad characters.
class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    int col = 1;

    struct ScanResult {
        bool ok;
        AcceptInfo info;
        size_t start;
        size_t end; // one past the last consumed byte
    };

    // runs the DFA from `start`, returns the longest match if there is one
    ScanResult scan_from(size_t start) const;

    void advance_over(const std::string& text);
};
