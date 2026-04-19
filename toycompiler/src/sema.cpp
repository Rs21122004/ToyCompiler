#include "sema.h"

void SemanticAnalyser::pushScope() { scopes_.push_back({}); }
void SemanticAnalyser::popScope()  { scopes_.pop_back(); }

void SemanticAnalyser::declareVar(const std::string& name) {
    auto& top = scopes_.back();
    if (top.count(name))
        throw std::runtime_error("Semantic error: variable '" + name +
                                 "' already declared in this scope.");
    top[name] = true;
}

void SemanticAnalyser::checkVar(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
        if (it->count(name)) return;
    throw std::runtime_error("Semantic error: variable '" + name +
                             "' used before declaration.");
}

void SemanticAnalyser::analyse(const std::vector<StmtPtr>& program) {
    pushScope();
    for (const auto& s : program) visitStmt(*s);
    popScope();
}

void SemanticAnalyser::visitStmt(const StmtNode& s) {
    switch (s.kind) {
        case StmtKind::VarDecl:
            if (s.initExpr) visitExpr(*s.initExpr);
            declareVar(s.varName);
            break;
        case StmtKind::ExprStmt:
            visitExpr(*s.expr);
            break;
        case StmtKind::Print:
            visitExpr(*s.printExpr);
            break;
        case StmtKind::If:
            visitExpr(*s.condition);
            visitStmt(*s.thenBranch);
            if (s.elseBranch) visitStmt(*s.elseBranch);
            break;
        case StmtKind::While:
            visitExpr(*s.whileCond);
            visitStmt(*s.whileBody);
            break;
        case StmtKind::Block:
            pushScope();
            for (const auto& child : s.stmts) visitStmt(*child);
            popScope();
            break;
    }
}

void SemanticAnalyser::visitExpr(const ExprNode& e) {
    switch (e.kind) {
        case ExprKind::IntLiteral: break;
        case ExprKind::Variable:
            checkVar(e.name);
            break;
        case ExprKind::Assign:
            checkVar(e.name);
            visitExpr(*e.left);
            break;
        case ExprKind::BinaryOp:
            visitExpr(*e.left);
            visitExpr(*e.right);
            break;
        case ExprKind::UnaryOp:
            visitExpr(*e.left);
            break;
    }
}