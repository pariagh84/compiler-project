#include "parser.hpp"

namespace {
int precedence(const std::string& op) {
    if (op == "=") return 1;
    if (op == "||") return 2;
    if (op == "&&") return 3;
    if (op == "==" || op == "!=") return 4;
    if (op == "<" || op == "<=" || op == ">" || op == ">=") return 5;
    if (op == "+" || op == "-") return 6;
    if (op == "*" || op == "/" || op == "%") return 7;
    if (op == "^") return 8;
    return 0;
}
}

Parser::Parser(const std::vector<Token>& input) {
    for (const auto& token : input) {
        if (token.kind != TokenKind::Whitespace && token.kind != TokenKind::Comment) tokens.push_back(token);
    }
    if (tokens.empty() || tokens.back().kind != TokenKind::End) {
        tokens.push_back({TokenKind::End, "", input.empty() ? Location{} : input.back().location});
    }
}

const Token& Parser::peek(std::size_t ahead) const {
    return tokens[std::min(current + ahead, tokens.size() - 1)];
}

Token Parser::take() {
    Token token = peek();
    if (token.kind != TokenKind::End) ++current;
    return token;
}

bool Parser::match(const std::string& text) {
    if (peek().lexeme != text) return false;
    take();
    return true;
}

Token Parser::expect(const std::string& text) {
    if (peek().lexeme != text) fail("expected '" + text + "', found '" + peek().lexeme + "'");
    return take();
}

void Parser::fail(const std::string& message) {
    diagnostics.push_back({peek().location, "parser", message});
    throw ParseError(message);
}

NodePtr Parser::expression(int minimum) {
    auto left = unary();
    while (precedence(peek().lexeme) >= minimum) {
        Token op = take();
        int level = precedence(op.lexeme);
        bool rightAssociative = op.lexeme == "=" || op.lexeme == "^";
        auto right = expression(level + (rightAssociative ? 0 : 1));
        auto parent = std::make_unique<Node>("BinaryExpr", op);
        parent->location = left->location;
        parent->children.push_back(std::move(left));
        parent->children.push_back(std::move(right));
        left = std::move(parent);
    }
    return left;
}

NodePtr Parser::unary() {
    if (peek().lexeme == "-" || peek().lexeme == "!") {
        auto node = std::make_unique<Node>("UnaryExpr", take());
        node->children.push_back(unary());
        return node;
    }
    return postfix();
}

NodePtr Parser::postfix() {
    auto node = primary();
    while (peek().lexeme == "(" || peek().lexeme == "[") {
        Token op = take();
        auto parent = std::make_unique<Node>(op.lexeme == "(" ? "CallExpr" : "IndexExpr", op);
        parent->location = node->location;
        parent->children.push_back(std::move(node));
        if (op.lexeme == "(") {
            if (peek().lexeme != ")") {
                do { parent->children.push_back(expression()); } while (match(","));
            }
            expect(")");
        } else {
            parent->children.push_back(expression());
            expect("]");
        }
        node = std::move(parent);
    }
    if (peek().lexeme == "++" || peek().lexeme == "--") {
        auto parent = std::make_unique<Node>("PostfixExpr", take());
        parent->location = node->location;
        parent->children.push_back(std::move(node));
        node = std::move(parent);
    }
    return node;
}

NodePtr Parser::primary() {
    Token token = peek();
    if (token.kind == TokenKind::Identifier) {
        take();
        return std::make_unique<Node>("Identifier", token);
    }
    if (token.kind == TokenKind::Integer || token.kind == TokenKind::Boolean ||
        token.kind == TokenKind::String || token.kind == TokenKind::Character) {
        take();
        return std::make_unique<Node>("Literal", token);
    }
    if (match("(")) {
        auto node = std::make_unique<Node>("GroupExpr", token);
        node->children.push_back(expression());
        expect(")");
        return node;
    }
    fail("expected an expression");
}
