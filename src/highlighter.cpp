#include "highlighter.hpp"
#include <map>
#include <set>

std::vector<std::string> classify(const std::vector<Token>& tokens, const Node& root,
                                const std::vector<Diagnostic>& diagnostics) {
    std::set<std::size_t> functions;
    std::set<std::size_t> errors;
    for (const auto& error : diagnostics) errors.insert(error.location.offset);
    std::vector<const Node*> pending{&root};
    while (!pending.empty()) {
        const Node* node = pending.back();
        pending.pop_back();
        if (node->kind == "FuncDecl" || (node->kind == "ParamDecl" &&
            !node->children.empty() && node->children[0]->token.lexeme == "function")) {
            functions.insert(node->token.location.offset);
        }
        if (node->kind == "CallExpr" && !node->children.empty()) {
            const Node* callee = node->children[0].get();
            while (callee->kind == "GroupExpr") callee = callee->children[0].get();
            if (callee->kind == "Identifier") functions.insert(callee->token.location.offset);
        }
        for (const auto& child : node->children) pending.push_back(child.get());
    }
    std::vector<std::string> styles;
    for (const auto& token : tokens) {
        std::string style;
        switch (token.kind) {
            case TokenKind::Keyword:
                style = token.lexeme == "integer" || token.lexeme == "boolean" || token.lexeme == "char" ||
                        token.lexeme == "string" || token.lexeme == "void" ? "type" : "kw";
                break;
            case TokenKind::Identifier: style = functions.count(token.location.offset) ? "fn" : "id"; break;
            case TokenKind::Integer:
            case TokenKind::Boolean: style = "num"; break;
            case TokenKind::String:
            case TokenKind::Character: style = "lit"; break;
            case TokenKind::Operator: style = "op"; break;
            case TokenKind::Comment: style = "comment"; break;
            case TokenKind::Invalid: style = "error"; break;
            default: break;
        }
        if (errors.count(token.location.offset) && token.kind != TokenKind::End) style = "error";
        styles.push_back(style);
    }
    return styles;
}

void writeAnsi(const std::vector<Token>& tokens, const std::vector<std::string>& styles, std::ostream& out) {
    const std::map<std::string, std::string> colors = {
        {"kw", "\033[1;34m"}, {"type", "\033[36m"}, {"fn", "\033[33m"},
        {"num", "\033[38;5;208m"}, {"lit", "\033[32m"}, {"op", "\033[37m"},
        {"comment", "\033[2;3;90m"}, {"error", "\033[4;31m"}
    };
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        auto color = colors.find(styles.at(i));
        if (color != colors.end()) out << color->second;
        out << tokens[i].lexeme;
        if (color != colors.end()) out << "\033[0m";
    }
}
