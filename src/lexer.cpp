#include "lexer.hpp"
#include <utility>

namespace {
bool letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}
bool digit(char c) { return c >= '0' && c <= '9'; }
bool space(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
bool keyword(const std::string& text) {
    const std::vector<std::string> words = {
        "array", "boolean", "char", "else", "for", "function", "if",
        "integer", "print", "return", "string", "void", "while"
    };
    for (const auto& word : words) if (text == word) return true;
    return false;
}
}

Lexer::Lexer(std::string source, std::string file) : source(std::move(source)) {
    position.file = std::move(file);
}

char Lexer::peek(std::size_t ahead) const {
    return position.offset + ahead < source.size() ? source[position.offset + ahead] : '\0';
}

void Lexer::advance() {
    if (position.offset >= source.size()) return;
    char c = source[position.offset++];
    if (c == '\r') {
        ++position.line;
        position.column = 1;
    } else if (c == '\n') {
        if (position.offset < 2 || source[position.offset - 2] != '\r') ++position.line;
        position.column = 1;
    } else {
        ++position.column;
    }
}

void Lexer::emit(TokenKind kind, const Location& start) {
    tokens.push_back({kind, source.substr(start.offset, position.offset - start.offset), start});
}

void Lexer::invalid(const Location& start, const std::string& message) {
    emit(TokenKind::Invalid, start);
    diagnostics.push_back({start, "lexer", message});
}

void Lexer::quoted(const Location& start, char quote) {
    advance();
    std::size_t length = 0;
    bool ascii = true;
    while (position.offset < source.size() && peek() != quote && peek() != '\n' && peek() != '\r') {
        if (peek() == '\\') {
            advance();
            if (position.offset == source.size() || peek() == '\n' || peek() == '\r') break;
        }
        if (static_cast<unsigned char>(peek()) > 127 || peek() == '\0') ascii = false;
        advance();
        ++length;
    }
    if (position.offset == source.size() || peek() != quote) {
        invalid(start, quote == '"' ? "unterminated string literal" : "unterminated character literal");
        return;
    }
    advance();
    if (!ascii) invalid(start, "literal must contain ASCII characters");
    else if (quote == '\'' && length != 1) invalid(start, "character literal must contain one character");
    else if (quote == '"' && length > 256) invalid(start, "string literal exceeds 256 characters");
    else emit(quote == '"' ? TokenKind::String : TokenKind::Character, start);
}

std::vector<Token> Lexer::scan() {
    // Reset so a second scan produces the same result.
    position.offset = 0;
    position.line = position.column = 1;
    tokens.clear();
    diagnostics.clear();
    while (position.offset < source.size()) {
        Location start = position;
        char c = peek();
        if (space(c)) {
            do { advance(); } while (space(peek()));
            emit(TokenKind::Whitespace, start);
        } else if (letter(c)) {
            do { advance(); } while (letter(peek()) || digit(peek()));
            std::string word = source.substr(start.offset, position.offset - start.offset);
            if (word.size() > 256) invalid(start, "identifier exceeds 256 characters");
            else if (word == "true" || word == "false") emit(TokenKind::Boolean, start);
            else emit(keyword(word) ? TokenKind::Keyword : TokenKind::Identifier, start);
        } else if (digit(c)) {
            do { advance(); } while (digit(peek()));
            emit(TokenKind::Integer, start);
        } else if (c == '"' || c == '\'') {
            quoted(start, c);
        } else if (c == '/' && peek(1) == '/') {
            while (position.offset < source.size() && peek() != '\n' && peek() != '\r') advance();
            emit(TokenKind::Comment, start);
        } else if (c == '/' && peek(1) == '*') {
            advance();
            advance();
            while (position.offset < source.size() && !(peek() == '*' && peek(1) == '/')) advance();
            if (position.offset == source.size()) invalid(start, "unterminated block comment");
            else {
                advance();
                advance();
                emit(TokenKind::Comment, start);
            }
        } else {
            std::string pair = source.substr(position.offset, 2);
            if (pair == "++" || pair == "--" || pair == "<=" || pair == ">=" ||
                pair == "==" || pair == "!=" || pair == "&&" || pair == "||") {
                advance();
                advance();
                emit(TokenKind::Operator, start);
            } else if (c != '\0' && std::string("+-*/%^!<>=").find(c) != std::string::npos) {
                advance();
                emit(TokenKind::Operator, start);
            } else if (c != '\0' && std::string("(){}[],:;").find(c) != std::string::npos) {
                advance();
                emit(TokenKind::Delimiter, start);
            } else {
                advance();
                invalid(start, "unknown character");
            }
        }
    }
    tokens.push_back({TokenKind::End, "", position});
    return tokens;
}
