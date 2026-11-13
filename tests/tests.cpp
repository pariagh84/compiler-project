#include "diagnostic.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    try {
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
