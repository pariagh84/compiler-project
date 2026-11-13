#include "highlighter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>

namespace {
void usage(std::ostream& out) {
    out << "Usage: bminor [--ansi|--tokens|--ast|--check] source.bminor\n"
        << "       bminor --html output.html source.bminor\n";
}

std::string visible(const std::string& text) {
    std::string result;
    for (unsigned char c : text) {
        if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else if (c == '\0') result += "\\0";
        else result += static_cast<char>(c);
    }
    return result;
}
}

int main(int argc, char* argv[]) {
    try {
        std::string mode = "--ansi";
        std::string inputPath;
        std::string outputPath;
        if (argc == 2 && std::string(argv[1]) == "--help") { usage(std::cout); return 0; }
        if (argc == 2 && std::string(argv[1]).rfind("--", 0) != 0) inputPath = argv[1];
        else if (argc == 3) { mode = argv[1]; inputPath = argv[2]; }
        else if (argc == 4 && std::string(argv[1]) == "--html") {
            mode = argv[1]; outputPath = argv[2]; inputPath = argv[3];
        } else { usage(std::cerr); return 2; }
        if (mode != "--ansi" && mode != "--tokens" && mode != "--ast" && mode != "--check" && mode != "--html") {
            usage(std::cerr);
            return 2;
        }
        if (mode == "--html" && outputPath.empty()) { usage(std::cerr); return 2; }
        std::ifstream input(inputPath, std::ios::binary);
        if (!input) throw std::runtime_error("cannot open input file: " + inputPath);
        std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (input.bad()) throw std::runtime_error("cannot read input file: " + inputPath);
        Lexer lexer(source, inputPath);
        auto tokens = lexer.scan();
        Parser parser(tokens);
        auto tree = parser.parse();
        auto diagnostics = lexer.errors();
        diagnostics.insert(diagnostics.end(), parser.errors().begin(), parser.errors().end());
        for (const auto& error : diagnostics) std::cerr << error << '\n';

        if (mode == "--tokens") {
            for (const auto& token : tokens) {
                std::cout << token.location.file << ':' << token.location.line << ':' << token.location.column
                          << ' ' << tokenName(token.kind) << ' ' << std::quoted(visible(token.lexeme)) << '\n';
            }
        } else if (mode == "--ast") printAst(*tree, std::cout);
        else if (mode != "--check") {
            auto styles = classify(tokens, *tree, diagnostics);
            if (mode == "--ansi") writeAnsi(tokens, styles, std::cout);
            else {
                if (std::filesystem::exists(outputPath) && std::filesystem::equivalent(inputPath, outputPath)) {
                    throw std::runtime_error("output file must differ from input file");
                }
                std::ofstream output(outputPath, std::ios::binary);
                if (!output) throw std::runtime_error("cannot open output file: " + outputPath);
                writeHtml(tokens, styles, output);
                output.close();
                if (!output) throw std::runtime_error("cannot write output file: " + outputPath);
            }
        }
        if (!std::cout) throw std::runtime_error("cannot write standard output");
        return diagnostics.empty() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "bminor: " << error.what() << '\n';
        return 2;
    }
}
