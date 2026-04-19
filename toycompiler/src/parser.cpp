#include "parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

Token& Parser::peek(int offset) {
    size_t idx = pos_ + offset;
    return tokens_[idx < tokens_.size() ? idx : tokens_.size() - 1];
}
Token& Parser::advance()                    { return tokens_[pos_++]; }
bool   Parser::check(TokenType t)    const  { return tokens_[pos_].type == t; }
bool   Parser::match(TokenType t)           {
    if (check(t)) { advance(); return true; } return false;
}
Token& Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t))
        throw std::runtime_error("Parse error at line " +
            std::to_string(peek().line) + ": " + msg +
            " (got '" + peek().lexeme + "')");
    return advance();
}

// ─── Program ─────────────────────────────────────────────────────────────────
std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::END_OF_FILE))
        stmts.push_back(parseStmt());
    return stmts;
}

// ─── Statements ──────────────────────────────────────────────────────────────
StmtPtr Parser::parseStmt() {
    if (check(TokenType::KW_INT))    return parseVarDecl();
    if (check(TokenType::KW_IF))     return parseIfStmt();
    if (check(TokenType::KW_WHILE))  return parseWhileStmt();
    if (check(TokenType::KW_PRINT))  return parsePrintStmt();
    if (check(TokenType::LBRACE))    return parseBlock();
    return parseExprStmt();
}

StmtPtr Parser::parseVarDecl() {
    expect(TokenType::KW_INT, "expected 'int'");
    std::string name = expect(TokenType::IDENTIFIER, "expected variable name").lexeme;
    ExprPtr init;
    if (match(TokenType::ASSIGN))
        init = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after variable declaration");
    return StmtNode::varDecl(name, std::move(init));
}

StmtPtr Parser::parseIfStmt() {
    expect(TokenType::KW_IF,    "expected 'if'");
    expect(TokenType::LPAREN,   "expected '(' after 'if'");
    auto cond = parseExpr();
    expect(TokenType::RPAREN,   "expected ')' after condition");
    auto thenB = parseBlock();
    StmtPtr elseB;
    if (match(TokenType::KW_ELSE))
        elseB = parseBlock();
    return StmtNode::ifStmt(std::move(cond), std::move(thenB), std::move(elseB));
}

StmtPtr Parser::parseWhileStmt() {
    expect(TokenType::KW_WHILE, "expected 'while'");
    expect(TokenType::LPAREN,   "expected '(' after 'while'");
    auto cond = parseExpr();
    expect(TokenType::RPAREN,   "expected ')' after condition");
    auto body = parseBlock();
    return StmtNode::whileStmt(std::move(cond), std::move(body));
}

StmtPtr Parser::parsePrintStmt() {
    expect(TokenType::KW_PRINT,   "expected 'print'");
    expect(TokenType::LPAREN,     "expected '(' after 'print'");
    auto e = parseExpr();
    expect(TokenType::RPAREN,     "expected ')' after expression");
    expect(TokenType::SEMICOLON,  "expected ';'");
    return StmtNode::printStmt(std::move(e));
}

StmtPtr Parser::parseExprStmt() {
    auto e = parseExpr();
    expect(TokenType::SEMICOLON, "expected ';' after expression");
    return StmtNode::exprStmt(std::move(e));
}

StmtPtr Parser::parseBlock() {
    expect(TokenType::LBRACE, "expected '{'");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
        stmts.push_back(parseStmt());
    expect(TokenType::RBRACE, "expected '}'");
    return StmtNode::block(std::move(stmts));
}

// ─── Expressions ─────────────────────────────────────────────────────────────
ExprPtr Parser::parseExpr()        { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
    // Look ahead: IDENT '=' -> assignment
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::ASSIGN) {
        std::string name = advance().lexeme;
        advance(); // consume '='
        auto val = parseAssignment();
        return ExprNode::assign(name, std::move(val));
    }
    return parseLogicOr();
}

ExprPtr Parser::parseLogicOr() {
    auto left = parseLogicAnd();
    while (check(TokenType::OR)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseLogicAnd());
    }
    return left;
}

ExprPtr Parser::parseLogicAnd() {
    auto left = parseEquality();
    while (check(TokenType::AND)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseEquality());
    }
    return left;
}

ExprPtr Parser::parseEquality() {
    auto left = parseRelational();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseRelational());
    }
    return left;
}

ExprPtr Parser::parseRelational() {
    auto left = parseAdditive();
    while (check(TokenType::LT)  || check(TokenType::LTE) ||
           check(TokenType::GT)  || check(TokenType::GTE)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseAdditive());
    }
    return left;
}

ExprPtr Parser::parseAdditive() {
    auto left = parseTerm();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseTerm());
    }
    return left;
}

ExprPtr Parser::parseTerm() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = advance().lexeme;
        left = ExprNode::binop(op, std::move(left), parseUnary());
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (check(TokenType::NOT) || check(TokenType::MINUS)) {
        std::string op = advance().lexeme;
        return ExprNode::unaryop(op, parseUnary());
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::INT_LITERAL)) {
        int v = std::stoi(advance().lexeme);
        return ExprNode::intLit(v);
    }
    if (check(TokenType::IDENTIFIER)) {
        return ExprNode::var(advance().lexeme);
    }
    if (match(TokenType::LPAREN)) {
        auto e = parseExpr();
        expect(TokenType::RPAREN, "expected ')' after expression");
        return e;
    }
    throw std::runtime_error("Parse error at line " +
        std::to_string(peek().line) + ": unexpected token '" + peek().lexeme + "'");
}