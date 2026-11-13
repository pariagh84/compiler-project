#pragma once

#include <cstddef>
#include <string>

struct Location {
    std::string file;
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

enum class TokenKind {
    Keyword, Identifier, Integer, Boolean, String, Character,
    Operator, Delimiter, Comment, Whitespace, Invalid, End
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    Location location;
};

inline const char* tokenName(TokenKind kind) {
    switch (kind) {
        case TokenKind::Keyword: return "KEYWORD";
        case TokenKind::Identifier: return "IDENTIFIER";
        case TokenKind::Integer: return "INTEGER";
        case TokenKind::Boolean: return "BOOLEAN";
        case TokenKind::String: return "STRING";
        case TokenKind::Character: return "CHARACTER";
        case TokenKind::Operator: return "OPERATOR";
        case TokenKind::Delimiter: return "DELIMITER";
        case TokenKind::Comment: return "COMMENT";
        case TokenKind::Whitespace: return "WHITESPACE";
        case TokenKind::Invalid: return "INVALID";
        case TokenKind::End: return "EOF";
    }
    return "UNKNOWN";
}
