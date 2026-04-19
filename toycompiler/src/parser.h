#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>

// ─── Recursive-descent parser ────────────────────────────────────────────────
// Grammar (simplified):
//
//   program     -> stmt*  EOF
//   stmt        -> varDecl | ifStmt | whileStmt | printStmt | exprStmt | block
//   varDecl     -> 'int' IDENT ('=' expr)? ';'
//   ifStmt      -> 'if' '(' expr ')' block ('else' block)?
//   whileStmt   -> 'while' '(' expr ')' block
//   printStmt   -> 'print' '(' expr ')' ';'
//   exprStmt    -> expr ';'
//   block       -> '{' stmt* '}'
//   expr        -> assignment
//   assignment  -> IDENT '=' assignment  |  logicOr
//   logicOr     -> logicAnd  ('||' logicAnd)*
//   logicAnd    -> equality  ('&&' equality)*
//   equality    -> relational (('=='|'!=') relational)*
//   relational  -> additive   (('<'|'<='|'>'|'>=') additive)*
//   additive    -> term       (('+'|'-') term)*
//   term        -> unary      (('*'|'/'|'%') unary)*
//   unary       -> '!' unary  |  '-' unary  |  primary
//   primary     -> INT_LITERAL | IDENT | '(' expr ')'

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::vector<StmtPtr> parse();

private:
    std::vector<Token> tokens_;
    size_t             pos_ = 0;

    Token&  peek(int offset = 0);
    Token&  advance();
    bool    check(TokenType t) const;
    bool    match(TokenType t);
    Token&  expect(TokenType t, const std::string& msg);

    // Statements
    StmtPtr parseStmt();
    StmtPtr parseVarDecl();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parsePrintStmt();
    StmtPtr parseExprStmt();
    StmtPtr parseBlock();

    // Expressions (Pratt-style via recursive descent)
    ExprPtr parseExpr();
    ExprPtr parseAssignment();
    ExprPtr parseLogicOr();
    ExprPtr parseLogicAnd();
    ExprPtr parseEquality();
    ExprPtr parseRelational();
    ExprPtr parseAdditive();
    ExprPtr parseTerm();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
};