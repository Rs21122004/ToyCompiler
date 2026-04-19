#include "lexer.h"
#include <stdexcept>
#include <cctype>

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"if",    TokenType::KW_IF},
    {"else",  TokenType::KW_ELSE},
    {"while", TokenType::KW_WHILE},
    {"int",   TokenType::KW_INT},
    {"print", TokenType::KW_PRINT},
};

Lexer::Lexer(std::string src) : src_(std::move(src)) {}

char Lexer::peek(int offset) const {
    size_t idx = pos_ + offset;
    return idx < src_.size() ? src_[idx] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') ++line_;
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < src_.size()) {
        char c = peek();
        if (std::isspace(c)) {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // line comment
            while (pos_ < src_.size() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            // block comment
            advance(); advance();
            while (pos_ < src_.size()) {
                if (peek() == '*' && peek(1) == '/') { advance(); advance(); break; }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    std::string num;
    int start_line = line_;
    while (pos_ < src_.size() && std::isdigit(peek()))
        num += advance();
    return {TokenType::INT_LITERAL, num, start_line};
}

Token Lexer::readIdentifierOrKeyword() {
    std::string word;
    int start_line = line_;
    while (pos_ < src_.size() && (std::isalnum(peek()) || peek() == '_'))
        word += advance();
    auto it = keywords_.find(word);
    TokenType t = (it != keywords_.end()) ? it->second : TokenType::IDENTIFIER;
    return {t, word, start_line};
}

Token Lexer::readOperatorOrDelimiter() {
    int start_line = line_;
    char c = advance();
    auto make = [&](TokenType t, std::string lex) { return Token{t, lex, start_line}; };

    switch (c) {
        case '+': return make(TokenType::PLUS,      "+");
        case '-': return make(TokenType::MINUS,     "-");
        case '*': return make(TokenType::STAR,      "*");
        case '/': return make(TokenType::SLASH,     "/");
        case '%': return make(TokenType::PERCENT,   "%");
        case '(': return make(TokenType::LPAREN,    "(");
        case ')': return make(TokenType::RPAREN,    ")");
        case '{': return make(TokenType::LBRACE,    "{");
        case '}': return make(TokenType::RBRACE,    "}");
        case ';': return make(TokenType::SEMICOLON, ";");
        case '=':
            if (peek() == '=') { advance(); return make(TokenType::EQ,     "=="); }
            return make(TokenType::ASSIGN, "=");
        case '!':
            if (peek() == '=') { advance(); return make(TokenType::NEQ,    "!="); }
            return make(TokenType::NOT, "!");
        case '<':
            if (peek() == '=') { advance(); return make(TokenType::LTE,    "<="); }
            return make(TokenType::LT, "<");
        case '>':
            if (peek() == '=') { advance(); return make(TokenType::GTE,    ">="); }
            return make(TokenType::GT, ">");
        case '&':
            if (peek() == '&') { advance(); return make(TokenType::AND,    "&&"); }
            break;
        case '|':
            if (peek() == '|') { advance(); return make(TokenType::OR,     "||"); }
            break;
        default: break;
    }
    throw std::runtime_error("Unknown character '" + std::string(1,c) +
                             "' at line " + std::to_string(start_line));
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (pos_ >= src_.size()) {
            tokens.emplace_back(TokenType::END_OF_FILE, "", line_);
            break;
        }
        char c = peek();
        if (std::isdigit(c))
            tokens.push_back(readNumber());
        else if (std::isalpha(c) || c == '_')
            tokens.push_back(readIdentifierOrKeyword());
        else
            tokens.push_back(readOperatorOrDelimiter());
    }
    return tokens;
}