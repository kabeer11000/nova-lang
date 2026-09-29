#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include "lexer.hpp"
#include "dfa.hpp"

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// no test framework wired up yet, just a quick sanity check
static void _demo() {
    Lexer lex("fn main() {\n    let mut x: i32 = 1 + 2\n    loop i in 0..10 { print(x) }\n}");
    auto tokens = lex.tokenize();

    assert(tokens.front().type == TokenType::Fn);
    assert(tokens[1].type == TokenType::Ident && tokens[1].text == "main");
    assert(tokens[2].type == TokenType::LParen);
    assert(tokens[3].type == TokenType::RParen);
    assert(tokens[4].type == TokenType::LBrace);

    bool saw_loop = false, saw_dotdot = false, saw_intlit_2 = false;
    for (auto& t : tokens) {
        if (t.type == TokenType::Loop) saw_loop = true;
        if (t.type == TokenType::DotDot) saw_dotdot = true;
        if (t.type == TokenType::IntLit && t.text == "2") saw_intlit_2 = true;
    }
    assert(saw_loop && saw_dotdot && saw_intlit_2);
    assert(tokens.back().type == TokenType::Eof);

    std::cout << "lexer self-check passed (" << tokens.size() << " tokens)\n";
}

// dumps the transition table as csv so it can be checked into docs/
static void dump_dfa() {
    const TransitionTable& table = dfa_table();

    std::cout << "state";
    for (size_t c = 0; c < (size_t)CharClass::Count; ++c) {
        std::cout << "," << char_class_name((CharClass)c);
    }
    std::cout << ",ACCEPTS\n";

    for (size_t s = 0; s < (size_t)DState::Count; ++s) {
        std::cout << dstate_name((DState)s);
        for (size_t c = 0; c < (size_t)CharClass::Count; ++c) {
            int next = table[s][c];
            std::cout << ",";
            if (next == DEAD) std::cout << "DEAD";
            else std::cout << dstate_name((DState)next);
        }
        AcceptInfo info = accepting_info((DState)s);
        std::cout << ",";
        if (info.kind == AcceptKind::Skip) std::cout << "SKIP";
        else if (info.kind == AcceptKind::Token) std::cout << token_type_name(info.token);
        std::cout << "\n";
    }
}

// just for printing -- keeps a decoded newline/tab from breaking the
// one-token-per-line output. Token::text itself still has the real bytes.
static std::string display_text(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\0': out += "\\0"; break;
            default: out += c; break;
        }
    }
    return out;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        _demo();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "--dump-dfa") {
        dump_dfa();
        return 0;
    }

    try {
        std::string source = read_file(argv[1]);
        Lexer lex(source);
        auto tokens = lex.tokenize();
        for (auto& t : tokens) {
            std::cout << "<" << token_type_name(t.type) << ", " << display_text(t.text) << ">\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
