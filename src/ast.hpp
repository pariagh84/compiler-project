#pragma once

#include "token.hpp"
#include <memory>
#include <optional>
#include <ostream>
#include <vector>

// Children keep their grammar order; each node owns its children.
struct Node {
    std::string kind;
    Token token;
    Location location;
    std::optional<std::string> inferredType;
    std::vector<std::unique_ptr<Node>> children;

    Node(std::string kind, const Token& token)
        : kind(std::move(kind)), token(token), location(token.location) {}
};

using NodePtr = std::unique_ptr<Node>;

void printAst(const Node& root, std::ostream& out);
