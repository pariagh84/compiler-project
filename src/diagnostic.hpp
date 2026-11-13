#pragma once

#include "token.hpp"
#include <ostream>

struct Diagnostic {
    Location location;
    std::string stage;
    std::string message;
};

inline std::ostream& operator<<(std::ostream& out, const Diagnostic& error) {
    return out << error.location.file << ':' << error.location.line << ':'
               << error.location.column << ": " << error.stage << ": " << error.message;
}
