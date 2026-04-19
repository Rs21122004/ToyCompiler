#pragma once
#include <memory>
#include <string>
#include <vector>

// ─── Forward declarations ────────────────────────────────────────────────────
struct ExprNode;
struct StmtNode;
using ExprPtr = std::unique_ptr<ExprNode>;
using StmtPtr = std::unique_ptr<StmtNode>;

// ═══════════════════════════════════════════════════════════════════════════
//  EXPRESSIONS
// ═══════════════════════════════════════════════════════════════════════════

enum class ExprKind {
    IntLiteral,
    Variable,
    BinaryOp,
    UnaryOp,
    Assign,
};

struct ExprNode {
    ExprKind    kind;
    // IntLiteral
    int         ival  = 0;
    // Variable / Assign target
    std::string name;
    // BinaryOp / UnaryOp
    std::string op;
    ExprPtr     left;
    ExprPtr     right;   // null for unary

    // Factory helpers
    static ExprPtr intLit(int v) {
        auto n = std::make_unique<ExprNode>();
        n->kind = ExprKind::IntLiteral; n->ival = v; return n;
    }
    static ExprPtr var(const std::string& nm) {
        auto n = std::make_unique<ExprNode>();
        n->kind = ExprKind::Variable; n->name = nm; return n;
    }
    static ExprPtr binop(std::string o, ExprPtr l, ExprPtr r) {
        auto n = std::make_unique<ExprNode>();
        n->kind = ExprKind::BinaryOp; n->op = std::move(o);
        n->left = std::move(l); n->right = std::move(r); return n;
    }
    static ExprPtr unaryop(std::string o, ExprPtr operand) {
        auto n = std::make_unique<ExprNode>();
        n->kind = ExprKind::UnaryOp; n->op = std::move(o);
        n->left = std::move(operand); return n;
    }
    static ExprPtr assign(const std::string& nm, ExprPtr val) {
        auto n = std::make_unique<ExprNode>();
        n->kind = ExprKind::Assign; n->name = nm; n->left = std::move(val); return n;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  STATEMENTS
// ═══════════════════════════════════════════════════════════════════════════

enum class StmtKind {
    VarDecl,    // int x;  or  int x = expr;
    ExprStmt,
    If,
    While,
    Block,
    Print,
};

struct StmtNode {
    StmtKind              kind;
    // VarDecl
    std::string           varName;
    ExprPtr               initExpr;   // optional
    // ExprStmt
    ExprPtr               expr;
    // If
    ExprPtr               condition;
    StmtPtr               thenBranch;
    StmtPtr               elseBranch; // optional
    // While
    ExprPtr               whileCond;
    StmtPtr               whileBody;
    // Block
    std::vector<StmtPtr>  stmts;
    // Print
    ExprPtr               printExpr;

    static StmtPtr varDecl(const std::string& nm, ExprPtr init = nullptr) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::VarDecl; n->varName = nm;
        n->initExpr = std::move(init); return n;
    }
    static StmtPtr exprStmt(ExprPtr e) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::ExprStmt; n->expr = std::move(e); return n;
    }
    static StmtPtr ifStmt(ExprPtr cond, StmtPtr thenB, StmtPtr elseB = nullptr) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::If; n->condition = std::move(cond);
        n->thenBranch = std::move(thenB); n->elseBranch = std::move(elseB); return n;
    }
    static StmtPtr whileStmt(ExprPtr cond, StmtPtr body) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::While; n->whileCond = std::move(cond);
        n->whileBody = std::move(body); return n;
    }
    static StmtPtr block(std::vector<StmtPtr> ss) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::Block; n->stmts = std::move(ss); return n;
    }
    static StmtPtr printStmt(ExprPtr e) {
        auto n = std::make_unique<StmtNode>();
        n->kind = StmtKind::Print; n->printExpr = std::move(e); return n;
    }
};