#include "parser.hpp"
#include <algorithm>

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

Parser::DepthGuard::DepthGuard(Parser& parser) : parser(parser) {
    if (parser.depth >= 128) parser.fail("nesting exceeds the analysis limit (128)");
    ++parser.depth;
}

Parser::DepthGuard::~DepthGuard() { --parser.depth; }

void Parser::attach(Node& parent, NodePtr child) {
    if (child->height >= 256) fail("AST depth exceeds the analysis limit (256)");
    parent.height = std::max(parent.height, child->height + 1);
    parent.children.push_back(std::move(child));
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
    DepthGuard guard(*this);
    auto left = unary();
    while (precedence(peek().lexeme) >= minimum) {
        Token op = take();
        int level = precedence(op.lexeme);
        bool rightAssociative = op.lexeme == "=" || op.lexeme == "^";
        auto right = expression(level + (rightAssociative ? 0 : 1));
        auto parent = std::make_unique<Node>("BinaryExpr", op);
        parent->location = left->location;
        attach(*parent, std::move(left));
        attach(*parent, std::move(right));
        left = std::move(parent);
    }
    return left;
}

NodePtr Parser::unary() {
    DepthGuard guard(*this);
    if (peek().lexeme == "-" || peek().lexeme == "!") {
        auto node = std::make_unique<Node>("UnaryExpr", take());
        attach(*node, unary());
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
        attach(*parent, std::move(node));
        if (op.lexeme == "(") {
            if (peek().lexeme != ")") {
                do { attach(*parent, expression()); } while (match(","));
            }
            expect(")");
        } else {
            attach(*parent, expression());
            expect("]");
        }
        node = std::move(parent);
    }
    if (peek().lexeme == "++" || peek().lexeme == "--") {
        auto parent = std::make_unique<Node>("PostfixExpr", take());
        parent->location = node->location;
        attach(*parent, std::move(node));
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
        attach(*node, expression());
        expect(")");
        return node;
    }
    fail("expected an expression");
}

NodePtr Parser::parse() {
    current = 0;
    diagnostics.clear();
    auto program = std::make_unique<Node>("Program", peek());
    while (peek().kind != TokenKind::End) {
        std::size_t start = current;
        try { attach(*program, declaration(true)); }
        catch (const ParseError&) {
            attach(*program, std::make_unique<Node>("Error", tokens[start]));
            synchronize(start, true);
        }
    }
    return program;
}

NodePtr Parser::type(bool parameter) {
    DepthGuard guard(*this);
    Token token = take();
    auto node = std::make_unique<Node>("Type", token);
    if (token.lexeme == "integer" || token.lexeme == "boolean" ||
        token.lexeme == "char" || token.lexeme == "string") return node;
    if (token.lexeme == "array") {
        expect("[");
        if (peek().lexeme == "]") {
            if (!parameter) fail("array size is required outside parameters");
            attach(*node, std::make_unique<Node>("Omitted", peek()));
        } else attach(*node, expression());
        expect("]");
        attach(*node, type(parameter));
        return node;
    }
    if (token.lexeme == "function") {
        if (peek().lexeme == "void") attach(*node, std::make_unique<Node>("Type", take()));
        else {
            if (peek().lexeme != "integer" && peek().lexeme != "boolean" &&
                peek().lexeme != "char" && peek().lexeme != "string") fail("expected an atomic return type or void");
            attach(*node, type());
        }
        expect("(");
        if (peek().lexeme != ")") {
            do {
                if (peek().kind != TokenKind::Identifier) fail("expected a parameter name");
                auto param = std::make_unique<Node>("ParamDecl", take());
                expect(":");
                attach(*param, type(true));
                attach(*node, std::move(param));
            } while (match(","));
        }
        expect(")");
        return node;
    }
    // Point the diagnostic at the invalid type instead of the following token.
    if (token.kind != TokenKind::End) --current;
    fail("expected a B-Minor type");
}

NodePtr Parser::initializer() {
    DepthGuard guard(*this);
    if (peek().lexeme != "{") return expression();
    auto node = std::make_unique<Node>("InitList", take());
    if (peek().lexeme != "}") {
        do { attach(*node, initializer()); } while (match(","));
    }
    expect("}");
    return node;
}

NodePtr Parser::declaration(bool global) {
    if (peek().kind != TokenKind::Identifier) fail("expected a declaration name");
    auto node = std::make_unique<Node>("VarDecl", take());
    expect(":");
    attach(*node, type());
    bool function = node->children[0]->token.lexeme == "function";
    if (function) node->kind = "FuncDecl";
    if (match("=")) {
        if (function) {
            if (!global) fail("function definitions cannot be nested");
            attach(*node, block());
            return node;
        }
        attach(*node, initializer());
    }
    expect(";");
    return node;
}

NodePtr Parser::block() {
    DepthGuard guard(*this);
    auto node = std::make_unique<Node>("BlockStmt", expect("{"));
    while (peek().kind != TokenKind::End && peek().lexeme != "}") {
        std::size_t start = current;
        try { attach(*node, statement()); }
        catch (const ParseError&) {
            attach(*node, std::make_unique<Node>("Error", tokens[start]));
            synchronize(start, false);
        }
    }
    expect("}");
    return node;
}

NodePtr Parser::statement() {
    DepthGuard guard(*this);
    Token token = peek();
    if (token.lexeme == "{") return block();
    if (token.kind == TokenKind::Identifier && peek(1).lexeme == ":") return declaration(false);
    if (match("if")) {
        auto node = std::make_unique<Node>("IfStmt", token);
        expect("(");
        attach(*node, expression());
        expect(")");
        attach(*node, statement());
        // An else always belongs to the nearest unmatched if.
        if (match("else")) attach(*node, statement());
        return node;
    }
    if (match("for")) {
        auto node = std::make_unique<Node>("ForStmt", token);
        expect("(");
        for (const std::string separator : {";", ";", ")"}) {
            if (peek().lexeme == separator) attach(*node, std::make_unique<Node>("Omitted", peek()));
            else attach(*node, expression());
            expect(separator);
        }
        attach(*node, statement());
        return node;
    }
    if (match("return") || match("print")) {
        auto node = std::make_unique<Node>(token.lexeme == "return" ? "ReturnStmt" : "PrintStmt", token);
        if (peek().lexeme != ";") {
            attach(*node, expression());
            if (token.lexeme == "print") {
                while (match(",")) attach(*node, expression());
            }
        }
        expect(";");
        return node;
    }
    if (match(";")) return std::make_unique<Node>("EmptyStmt", token);
    auto node = std::make_unique<Node>("ExprStmt", token);
    attach(*node, expression());
    expect(";");
    return node;
}

void Parser::synchronize(std::size_t start, bool global) {
    int braces = 0;
    for (std::size_t i = start; i < current; ++i) {
        if (tokens[i].lexeme == "{") ++braces;
        if (tokens[i].lexeme == "}" && braces > 0) --braces;
    }
    while (peek().kind != TokenKind::End) {
        const std::string text = peek().lexeme;
        if (braces == 0) {
            if (text == ";") { take(); return; }
            if (text == "}") {
                if (global) take();
                return;
            }
            bool declarationStart = peek().kind == TokenKind::Identifier && peek(1).lexeme == ":";
            bool statementStart = text == "if" || text == "for" || text == "return" || text == "print";
            if (current > start && (declarationStart || (!global && statementStart))) return;
        }
        take();
        if (text == "{") ++braces;
        if (text == "}" && braces > 0) {
            --braces;
            if (braces == 0) return;
        }
    }
}
