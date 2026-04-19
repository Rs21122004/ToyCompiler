#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ─── Token types ─────────────────────────────────────────────────────────────
enum class TokenType {
    // Literals / identifiers
    INT_LITERAL, IDENTIFIER,
    // Keywords
    KW_IF, KW_ELSE, KW_WHILE, KW_INT, KW_PRINT,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NEQ, LT, LTE, GT, GTE,
    ASSIGN,
    AND, OR, NOT,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON,
    // Special
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    int         line;

    Token(TokenType t, std::string lex, int ln)
        : type(t), lexeme(std::move(lex)), line(ln) {}
};

// ─── Lexer ───────────────────────────────────────────────────────────────────
class Lexer {
public:
    explicit Lexer(std::string src);
    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t      pos_  = 0;
    int         line_ = 1;

    static const std::unordered_map<std::string, TokenType> keywords_;

    char        peek(int offset = 0) const;
    char        advance();
    void        skipWhitespaceAndComments();
    Token       readNumber();
    Token       readIdentifierOrKeyword();
    Token       readOperatorOrDelimiter();
};