#include "highlighter.hpp"
#include <map>
#include <set>

namespace {
std::string escapeHtml(const std::string& text) {
    std::string result;
    for (char c : text) {
        switch (c) {
            case '&': result += "&amp;"; break;
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '"': result += "&quot;"; break;
            case '\'': result += "&#39;"; break;
            case '\r': result += "&#13;"; break;
            case '\0': result += "&#xfffd;"; break;
            default: result += c;
        }
    }
    return result;
}
}

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

void writeHtml(const std::vector<Token>& tokens, const std::vector<std::string>& styles, std::ostream& out) {
    out << R"(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>B-Minor Syntax Highlighting</title>
<style>
body{margin:0;padding:32px;background:#141820;color:#e6edf3;font-family:system-ui,sans-serif}
main{max-width:1100px;margin:auto}h1{font-size:24px;font-weight:600;margin:0 0 8px}
p{color:#98a4b4;font-size:14px}pre{padding:24px;border:1px solid #303948;border-radius:12px;
background:#1b202a;overflow:auto;tab-size:4;line-height:1.7;font-size:15px}
code{font-family:ui-monospace,Menlo,Consolas,monospace}
.kw{color:#78a9ff;font-weight:bold}.type{color:#5ed5ce}.fn{color:#f2cf66}
.num{color:#ffa766}.lit{color:#a8d889}.op{color:#c8d0db}.id{color:#e6edf3}
.comment{color:#929aaa;font-style:italic}.error{color:#ff7c86;text-decoration:underline wavy}
</style></head><body><main><h1>B-Minor</h1>
<p>Syntax highlighting</p><pre><code>)";
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (tokens[i].kind == TokenKind::End) continue;
        const auto& style = styles.at(i);
        if (!style.empty()) out << "<span class=\"" << style << "\">";
        out << escapeHtml(tokens[i].lexeme);
        if (!style.empty()) out << "</span>";
    }
    out << "</code></pre></main></body></html>\n";
}
