#pragma once
#include "nova/lexer/token.h"
#include <string_view>
#include <vector>
#include <unordered_map>

namespace nova {

class Lexer {
public:
    explicit Lexer(std::string_view source);
    
    std::vector<Token> tokenize();

private:
    Token nextToken();
    void skipWhitespaceAndComments();
    
    Token identifierOrKeyword();
    Token number();
    Token string();
    Token character();
    
    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;
    bool match(char expected);

    Token makeToken(TokenType type);
    Token makeErrorToken(std::string_view message);

    std::string_view source_;
    size_t start_ = 0;
    size_t current_ = 0;
    uint32_t line_ = 1;
    uint32_t column_ = 1;
    uint32_t token_start_col_ = 1;
    
    static const std::unordered_map<std::string_view, TokenType> keywords;
};

} // namespace nova
