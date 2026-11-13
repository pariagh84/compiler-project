#pragma once

#include "diagnostic.hpp"
#include <vector>

class Lexer {
public:
    Lexer(std::string source, std::string file);
    std::vector<Token> scan();
    const std::vector<Diagnostic>& errors() const { return diagnostics; }

private:
    std::string source;
    Location position;
    std::vector<Token> tokens;
    std::vector<Diagnostic> diagnostics;

    char peek(std::size_t ahead = 0) const;
    void advance();
    void emit(TokenKind kind, const Location& start);
    void invalid(const Location& start, const std::string& message);
    void quoted(const Location& start, char quote);
};
