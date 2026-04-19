#pragma once
#include "ast.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <stdexcept>

// ─── Symbol table ─────────────────────────────────────────────────────────────
// Single type system: everything is a 32-bit int (like PTX .s32)
// Scopes are stacked; each scope is a map from name -> declared (bool)

class SemanticAnalyser {
public:
    void analyse(const std::vector<StmtPtr>& program);

private:
    // Scope stack — each level maps variable name to true (declared)
    std::vector<std::unordered_map<std::string, bool>> scopes_;

    void pushScope();
    void popScope();
    void declareVar(const std::string& name);
    void checkVar(const std::string& name);

    void visitStmt(const StmtNode& s);
    void visitExpr(const ExprNode& e);
};