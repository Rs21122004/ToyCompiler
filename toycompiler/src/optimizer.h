#pragma once
#include "ast.h"
#include <vector>

// ─── Constant Folding Optimisation Pass ──────────────────────────────────────
//
// Walks the AST before codegen and folds constant expressions at compile time.
// Examples:
//   2 + 3        -> 5
//   10 * 0       -> 0
//   !(0)         -> 1
//   x + 0        -> x       (identity elimination)
//   x * 1        -> x       (identity elimination)
//   x * 0        -> 0       (zero annihilation)
//
// This is a classic compiler optimisation — the same kind of pass that runs
// in real compilers (GCC, LLVM, PTXAS) before register allocation.

class ConstantFolder {
public:
    // Transforms the program in-place; returns number of folds applied.
    int fold(std::vector<StmtPtr>& program);

private:
    int foldCount_ = 0;

    void  foldStmt(StmtPtr& s);
    // Returns a folded replacement node, or nullptr if no fold was possible
    ExprPtr foldExpr(ExprPtr& e);

    // Attempt to evaluate a binary op on two known integer values
    static bool evalBinop(const std::string& op, int l, int r, int& result);
};