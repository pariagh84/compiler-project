#include "ast.hpp"
#include <iomanip>

void printAst(const Node& root, std::ostream& out) {
    struct Entry { const Node* node; std::size_t depth; };
    std::vector<Entry> pending{{&root, 0}};
    while (!pending.empty()) {
        auto entry = pending.back();
        pending.pop_back();
        out << std::string(entry.depth * 2, ' ') << entry.node->kind
            << ' ' << std::quoted(entry.node->token.lexeme)
            << " @ " << entry.node->location.line << ':' << entry.node->location.column << '\n';
        for (auto it = entry.node->children.rbegin(); it != entry.node->children.rend(); ++it) {
            pending.push_back({it->get(), entry.depth + 1});
        }
    }
}
