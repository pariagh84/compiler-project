#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"

std::vector<std::string> classify(const std::vector<Token>& tokens, const Node& root,
                                const std::vector<Diagnostic>& diagnostics);
void writeAnsi(const std::vector<Token>& tokens, const std::vector<std::string>& styles, std::ostream& out);
