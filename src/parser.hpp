#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include <stdexcept>

class Parser {
public:
    explicit Parser(const std::vector<Token>& input);
    NodePtr expression(int minimum = 1);
    NodePtr parse();
    const std::vector<Diagnostic>& errors() const { return diagnostics; }

private:
    struct ParseError : std::runtime_error { using std::runtime_error::runtime_error; };
    std::vector<Token> tokens;
    std::size_t current = 0;
    std::vector<Diagnostic> diagnostics;

    const Token& peek(std::size_t ahead = 0) const;
    Token take();
    bool match(const std::string& text);
    Token expect(const std::string& text);
    [[noreturn]] void fail(const std::string& message);
    NodePtr unary();
    NodePtr postfix();
    NodePtr primary();
    NodePtr type(bool parameter = false);
    NodePtr declaration(bool global);
    NodePtr statement();
    NodePtr block();
    NodePtr initializer();
};
