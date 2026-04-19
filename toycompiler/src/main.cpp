#include "lexer.h"
#include "parser.h"
#include "sema.h"
#include "optimizer.h"
#include "codegen.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "ToyCompiler — Simple Language -> PTX Backend\n\n"
                  << "Usage: toyc <source.toy> [flags]\n\n"
                  << "Flags:\n"
                  << "  --emit-tokens   Dump token stream after lexing\n"
                  << "  --no-opt        Disable constant folding pass\n";
        return 1;
    }

    std::string sourcePath = argv[1];
    bool emitTokens = false;
    bool noOpt      = false;
    for (int i = 2; i < argc; ++i) {
        std::string flag = argv[i];
        if (flag == "--emit-tokens") emitTokens = true;
        if (flag == "--no-opt")      noOpt      = true;
    }

    try {
        // ── 1. Read source ────────────────────────────────────────────────
        std::string source = readFile(sourcePath);
        std::cout << "=== Source ===\n" << source << "\n";

        // ── 2. Lexing ─────────────────────────────────────────────────────
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (emitTokens) {
            std::cout << "=== Tokens ===\n";
            for (const auto& t : tokens)
                std::cout << "  [line " << t.line << "] " << t.lexeme << "\n";
            std::cout << "\n";
        }

        // ── 3. Parsing ────────────────────────────────────────────────────
        Parser parser(std::move(tokens));
        auto ast = parser.parse();
        std::cout << "=== Parse:  OK (" << ast.size() << " top-level statements) ===\n";

        // ── 4. Semantic analysis ──────────────────────────────────────────
        SemanticAnalyser sema;
        sema.analyse(ast);
        std::cout << "=== Sema:   OK (scope analysis, use-before-declare checks) ===\n";

        // ── 5. Optimisation passes ────────────────────────────────────────
        if (!noOpt) {
            ConstantFolder folder;
            int folds = folder.fold(ast);
            std::cout << "=== Opt:    OK (constant folding — " << folds << " fold(s) applied) ===\n";
        } else {
            std::cout << "=== Opt:    SKIPPED (--no-opt) ===\n";
        }

        // ── 6. PTX code generation ────────────────────────────────────────
        Codegen cg;
        std::string ptx = cg.generate(ast);
        std::cout << "\n=== PTX Output ===\n" << ptx << "\n";

        // ── 7. Write .ptx file ────────────────────────────────────────────
        std::string outPath = sourcePath.substr(0, sourcePath.rfind('.')) + ".ptx";
        std::ofstream out(outPath);
        out << ptx;
        std::cout << "=== Written to: " << outPath << " ===\n";

    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}