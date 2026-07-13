#include "nova/lexer/lexer.h"
#include <cctype>

namespace nova {

const std::unordered_map<std::string_view, TokenType> Lexer::keywords = {
    {"int", TokenType::Int},
    {"float", TokenType::Float},
    {"bool", TokenType::Bool},
    {"string", TokenType::String},
    {"char", TokenType::Char},
    {"let", TokenType::Let},
    {"fn", TokenType::Fn},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"for", TokenType::For},
    {"break", TokenType::Break},
    {"continue", TokenType::Continue},
    {"return", TokenType::Return},
    {"struct", TokenType::Struct},
    {"enum", TokenType::Enum},
    {"module", TokenType::Module},
    {"import", TokenType::Import},
    {"true", TokenType::True},
    {"false", TokenType::False},
};

Lexer::Lexer(std::string_view source) : source_(source) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (!isAtEnd()) {
        start_ = current_;
        token_start_col_ = column_;
        Token token = nextToken();
        if (token.type != TokenType::Error || !token.lexeme.empty()) {
            tokens.push_back(token);
        }
    }
    tokens.push_back(makeToken(TokenType::Eof));
    return tokens;
}

Token Lexer::nextToken() {
    skipWhitespaceAndComments();
    if (isAtEnd()) return makeToken(TokenType::Eof);

    start_ = current_;
    token_start_col_ = column_;
    
    char c = advance();

    if (std::isalpha(c) || c == '_') return identifierOrKeyword();
    if (std::isdigit(c)) return number();

    switch (c) {
        case '(': return makeToken(TokenType::LParen);
        case ')': return makeToken(TokenType::RParen);
        case '{': return makeToken(TokenType::LBrace);
        case '}': return makeToken(TokenType::RBrace);
        case '[': return makeToken(TokenType::LBracket);
        case ']': return makeToken(TokenType::RBracket);
        case ',': return makeToken(TokenType::Comma);
        case '.': return makeToken(TokenType::Dot);
        case ';': return makeToken(TokenType::Semicolon);
        case ':': return makeToken(TokenType::Colon);
        case '+': return makeToken(TokenType::Plus);
        case '-': return match('>') ? makeToken(TokenType::Arrow) : makeToken(TokenType::Minus);
        case '*': return makeToken(TokenType::Star);
        case '/': return makeToken(TokenType::Slash);
        case '%': return makeToken(TokenType::Percent);
        case '!': return match('=') ? makeToken(TokenType::BangEq) : makeToken(TokenType::Bang);
        case '=': return match('=') ? makeToken(TokenType::EqEq) : makeToken(TokenType::Eq);
        case '<': return match('=') ? makeToken(TokenType::LessEq) : makeToken(TokenType::Less);
        case '>': return match('=') ? makeToken(TokenType::GreaterEq) : makeToken(TokenType::Greater);
        case '&':
            if (match('&')) return makeToken(TokenType::AndAnd);
            return makeErrorToken("Unexpected character '&'");
        case '|':
            if (match('|')) return makeToken(TokenType::OrOr);
            return makeErrorToken("Unexpected character '|'");
        case '"': return string();
        case '\'': return character();
        default:
            return makeErrorToken("Unexpected character");
    }
}

void Lexer::skipWhitespaceAndComments() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                line_++;
                column_ = 0;
                advance();
                column_ = 1; // It becomes 1 after advance
                break;
            case '/':
                if (peekNext() == '/') {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token Lexer::identifierOrKeyword() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    std::string_view text = source_.substr(start_, current_ - start_);
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return makeToken(it->second);
    }
    return makeToken(TokenType::Identifier);
}

Token Lexer::number() {
    bool isFloat = false;
    while (std::isdigit(peek())) advance();

    if (peek() == '.' && std::isdigit(peekNext())) {
        isFloat = true;
        advance(); // Consume '.'
        while (std::isdigit(peek())) advance();
    }

    return makeToken(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral);
}

Token Lexer::string() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') {
            line_++;
            column_ = 0;
        }
        advance();
        if (peek() == '\n') { // The newline case
            column_ = 1;
        }
    }

    if (isAtEnd()) return makeErrorToken("Unterminated string");

    advance(); // Consume closing quote
    return makeToken(TokenType::StringLiteral);
}

Token Lexer::character() {
    if (peek() == '\\') {
        advance(); // Consume escape character
        advance(); // Consume escaped character
    } else {
        advance();
    }

    if (peek() != '\'') return makeErrorToken("Unterminated character literal");

    advance(); // Consume closing quote
    return makeToken(TokenType::CharLiteral);
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char Lexer::advance() {
    current_++;
    column_++;
    return source_[current_ - 1];
}

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    current_++;
    column_++;
    return true;
}

Token Lexer::makeToken(TokenType type) {
    return Token{
        type,
        source_.substr(start_, current_ - start_),
        SourceLocation{line_, token_start_col_}
    };
}

Token Lexer::makeErrorToken(std::string_view message) {
    return Token{
        TokenType::Error,
        message, 
        SourceLocation{line_, token_start_col_}
    };
}

} // namespace nova
