#include "diagnostic.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include "highlighter.hpp"
#include <regex>
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

void parserTests() {
    Lexer lexer(R"(
puts: function void(s: string);
data: array [2] array [2] integer = {{1,2},{3,4}};
main: function integer() = {
    i: integer = 0;
    for(i=0; i<2; i++) { print data[i][0], "\n"; }
    for(;;) if(true) if(false) return 1; else return 2;
    puts("done");
    return -2^2;
}


)", "program");
    Parser parser(lexer.scan());
    auto tree = parser.parse();
    check(parser.errors().empty(), "valid declarations and statements");
    check(tree->children.size() == 3, "all global declarations retained");
    auto& body = tree->children[2]->children[1];
    check(body->children[2]->children[3]->children[1]->children.size() == 3, "else binds to inner if");
    auto& power = body->children.back()->children[0];
    check(power->kind == "BinaryExpr" && power->children[0]->kind == "UnaryExpr", "unary negation binds before exponentiation");

    Lexer broken("main: function integer() = { x: integer = ; @; return 7; } later: integer;", "broken");
    Parser recovery(broken.scan());
    auto recovered = recovery.parse();
    check(recovery.errors().size() == 2, "report multiple syntax errors");
    check(recovered->children.back()->token.lexeme == "later", "recover later global declaration");
    check(recovered->children[0]->children[1]->children.back()->kind == "ReturnStmt", "recover later statement");
    Lexer missing("main: function void() = { x = 1 return; }", "missing");
    Parser missingParser(missing.scan());
    auto missingTree = missingParser.parse();
    check(missingTree->children[0]->children[1]->children.back()->kind == "ReturnStmt", "synchronize on return after missing semicolon");
    Lexer deep("x: integer = " + std::string(5000, '(') + "1" + std::string(5000, ')') + "; y: integer;", "deep");
    Parser deepParser(deep.scan());
    auto deepTree = deepParser.parse();
    check(!deepParser.errors().empty() && deepTree->children.back()->token.lexeme == "y", "bounded recursion and recovery");
    std::string chain = "x: integer = 0";
    for (int i = 0; i < 5000; ++i) chain += "+1";
    Lexer longLexer(chain + "; y: integer;", "chain");
    Parser longParser(longLexer.scan());
    auto longTree = longParser.parse();
    check(!longParser.errors().empty() && longTree->children.back()->token.lexeme == "y", "bounded left-associated AST depth");
}

void highlighterTests() {
    std::string source = "f: function void(); main: function integer() = { f(); x = f; (f)(); return 0; }\n";
    Lexer lexer(source, "color");
    auto tokens = lexer.scan();
    Parser parser(tokens);
    auto tree = parser.parse();
    auto styles = classify(tokens, *tree, {});
    std::vector<std::string> fStyles;
    for (std::size_t i = 0; i < tokens.size(); ++i) if (tokens[i].lexeme == "f") fStyles.push_back(styles[i]);
    check(fStyles == std::vector<std::string>({"fn", "fn", "id", "fn"}), "AST distinguishes calls from variable references");
    std::ostringstream ansi;
    writeAnsi(tokens, styles, ansi);
    check(std::regex_replace(ansi.str(), std::regex("\033\\[[0-9;]*m"), "") == source, "ANSI preserves original source");
}

int main() {
    try {
        lexerTests();
        parserTests();
        highlighterTests();
        Lexer expressions("a = b = 1 + 2 * 3 ^ 4 ^ 5", "expr");
        Parser expressionParser(expressions.scan());
        auto tree = expressionParser.expression();
        check(tree->token.lexeme == "=" && tree->children[1]->token.lexeme == "=", "right associative assignment");
        auto& sum = tree->children[1]->children[1];
        check(sum->token.lexeme == "+" && sum->children[1]->token.lexeme == "*", "multiplication before addition");
        auto& power = sum->children[1]->children[1];
        check(power->token.lexeme == "^" && power->children[1]->token.lexeme == "^", "right associative power");
        check(sum->location.column == 9, "binary node starts at its left operand");
        Node root("Program", {TokenKind::End, "", {"ast", 0, 1, 1}});
        root.children.push_back(std::make_unique<Node>("Identifier", Token{TokenKind::Identifier, "x", {"ast", 0, 1, 1}}));
        check(!root.inferredType.has_value(), "semantic type starts unset");
        std::ostringstream ast;
        printAst(root, ast);
        check(ast.str().find("Identifier \"x\" @ 1:1") != std::string::npos, "AST inspection output");
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
