#include "diagnostic.hpp"
#include "lexer.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void lexerTests() {
    std::string source = "return returning <= < == = ++ + && || true false\r\n\t// text\n/* block */";
    Lexer lexer(source, "lexer.bminor");
    auto all = lexer.scan();
    std::vector<Token> tokens;
    std::string rebuilt;
    for (const auto& token : all) {
        rebuilt += token.lexeme;
        if (token.kind != TokenKind::Whitespace && token.kind != TokenKind::Comment) tokens.push_back(token);
    }
    check(rebuilt == source, "lexer must preserve every source byte");
    check(tokens[0].kind == TokenKind::Keyword && tokens[1].kind == TokenKind::Identifier, "keyword boundary");
    check(tokens[2].lexeme == "<=" && tokens[3].lexeme == "<", "longest operator match");
    check(tokens[10].kind == TokenKind::Boolean, "boolean category");
    check(all.back().location.line == 3, "CRLF must count as one newline");
    check(lexer.errors().empty(), "valid input must have no lexer errors");

    Lexer invalid("@ x: integer; \"broken\nnext: char = 'ab'; /* open", "bad.bminor");
    auto bad = invalid.scan();
    check(invalid.errors().size() == 4, "unknown character and unterminated literal/comment diagnostics");
    check(bad[0].kind == TokenKind::Invalid && bad[2].lexeme == "x", "continue after invalid token");
    Lexer escapes("\"a\\n\\0\\q\\\"\" '\\n' '\\0' '\\q' '\\\\'", "escapes");
    escapes.scan();
    check(escapes.errors().empty(), "B-Minor allows a backslash before any ASCII character");
    Lexer limits(std::string(256, 'a') + " " + std::string(257, 'b') + " \"" + std::string(257, 'c') + "\"", "limits");
    limits.scan();
    check(limits.errors().size() == 2, "identifier and string length limits");
    Lexer zero(std::string("\0x", 2), "zero");
    check(zero.scan()[0].kind == TokenKind::Invalid, "embedded NUL is not EOF");
}

int main() {
    try {
        lexerTests();
        Token token{TokenKind::Identifier, "name", {"test.bminor", 0, 1, 1}};
        check(token.lexeme == "name", "token must preserve its text");
        std::ostringstream out;
        out << Diagnostic{token.location, "lexer", "unknown character"};
        check(out.str() == "test.bminor:1:1: lexer: unknown character", "diagnostic location");
        std::cout << "All tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
