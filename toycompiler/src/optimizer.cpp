#include "optimizer.h"
#include <stdexcept>

// ─── Public entry point ───────────────────────────────────────────────────────

int ConstantFolder::fold(std::vector<StmtPtr>& program) {
    foldCount_ = 0;
    for (auto& s : program) foldStmt(s);
    return foldCount_;
}

// ─── Statement walker ─────────────────────────────────────────────────────────

void ConstantFolder::foldStmt(StmtPtr& s) {
    switch (s->kind) {
        case StmtKind::VarDecl:
            if (s->initExpr) foldExpr(s->initExpr);
            break;
        case StmtKind::ExprStmt:
            foldExpr(s->expr);
            break;
        case StmtKind::Print:
            foldExpr(s->printExpr);
            break;
        case StmtKind::If:
            foldExpr(s->condition);
            foldStmt(s->thenBranch);
            if (s->elseBranch) foldStmt(s->elseBranch);
            break;
        case StmtKind::While:
            foldExpr(s->whileCond);
            foldStmt(s->whileBody);
            break;
        case StmtKind::Block:
            for (auto& child : s->stmts) foldStmt(child);
            break;
    }
}

// ─── Expression folder ────────────────────────────────────────────────────────
// Mutates `e` in place; returns the (possibly-replaced) node.

ExprPtr ConstantFolder::foldExpr(ExprPtr& e) {
    if (!e) return nullptr;

    // Bottom-up: fold children first
    if (e->left)  foldExpr(e->left);
    if (e->right) foldExpr(e->right);

    // ── Unary constant folding ────────────────────────────────────────────────
    if (e->kind == ExprKind::UnaryOp && e->left->kind == ExprKind::IntLiteral) {
        int val = e->left->ival;
        int result = 0;
        bool folded = true;
        if      (e->op == "-") result = -val;
        else if (e->op == "!") result = (val == 0) ? 1 : 0;
        else folded = false;

        if (folded) {
            ++foldCount_;
            e = ExprNode::intLit(result);
        }
        return nullptr;
    }

    // ── Binary constant folding ───────────────────────────────────────────────
    if (e->kind == ExprKind::BinaryOp) {
        bool leftIsLit  = e->left  && e->left->kind  == ExprKind::IntLiteral;
        bool rightIsLit = e->right && e->right->kind == ExprKind::IntLiteral;

        // Full constant fold: both operands are literals
        if (leftIsLit && rightIsLit) {
            int result = 0;
            if (evalBinop(e->op, e->left->ival, e->right->ival, result)) {
                ++foldCount_;
                e = ExprNode::intLit(result);
                return nullptr;
            }
        }

        // Identity / annihilation rules (algebraic simplifications)
        if (rightIsLit) {
            int rval = e->right->ival;
            // x + 0 -> x,  x - 0 -> x
            if ((e->op == "+" || e->op == "-") && rval == 0) {
                ++foldCount_;
                e = std::move(e->left);
                return nullptr;
            }
            // x * 1 -> x,  x / 1 -> x
            if ((e->op == "*" || e->op == "/") && rval == 1) {
                ++foldCount_;
                e = std::move(e->left);
                return nullptr;
            }
            // x * 0 -> 0
            if (e->op == "*" && rval == 0) {
                ++foldCount_;
                e = ExprNode::intLit(0);
                return nullptr;
            }
        }
        if (leftIsLit) {
            int lval = e->left->ival;
            // 0 + x -> x
            if (e->op == "+" && lval == 0) {
                ++foldCount_;
                e = std::move(e->right);
                return nullptr;
            }
            // 0 * x -> 0
            if (e->op == "*" && lval == 0) {
                ++foldCount_;
                e = ExprNode::intLit(0);
                return nullptr;
            }
            // 1 * x -> x
            if (e->op == "*" && lval == 1) {
                ++foldCount_;
                e = std::move(e->right);
                return nullptr;
            }
        }
    }

    return nullptr;
}

// ─── Arithmetic evaluator ─────────────────────────────────────────────────────

bool ConstantFolder::evalBinop(const std::string& op, int l, int r, int& result) {
    if (op == "+")  { result = l + r;                        return true; }
    if (op == "-")  { result = l - r;                        return true; }
    if (op == "*")  { result = l * r;                        return true; }
    if (op == "/")  { if (r == 0) return false; result = l / r; return true; }
    if (op == "%")  { if (r == 0) return false; result = l % r; return true; }
    if (op == "==") { result = (l == r) ? 1 : 0;             return true; }
    if (op == "!=") { result = (l != r) ? 1 : 0;             return true; }
    if (op == "<")  { result = (l <  r) ? 1 : 0;             return true; }
    if (op == "<=") { result = (l <= r) ? 1 : 0;             return true; }
    if (op == ">")  { result = (l >  r) ? 1 : 0;             return true; }
    if (op == ">=") { result = (l >= r) ? 1 : 0;             return true; }
    if (op == "&&") { result = (l && r) ? 1 : 0;             return true; }
    if (op == "||") { result = (l || r) ? 1 : 0;             return true; }
    return false;
}