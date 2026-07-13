#pragma once
#include <string_view>
#include <cstdint>
#include <string>

namespace nova {

enum class TokenType {
    // Keywords
    Int, Float, Bool, String, Char,
    Let, Fn, If, Else, While, For, Break, Continue, Return,
    Struct, Enum, Module, Import,

    // Identifiers and Literals
    Identifier,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,
    True, False,

    // Operators
    Plus, Minus, Star, Slash, Percent, // + - * / %
    EqEq, BangEq, Greater, Less, GreaterEq, LessEq, // == != > < >= <=
    Eq, // =
    AndAnd, OrOr, Bang, // && || !
    Arrow, // ->

    // Separators
    LParen, RParen, LBrace, RBrace, LBracket, RBracket, // () {} []
    Comma, Colon, Semicolon, Dot,

    // Special
    Eof,
    Error
};

struct SourceLocation {
    uint32_t line = 1;
    uint32_t column = 1;
};

struct Token {
    TokenType type;
    std::string_view lexeme;
    SourceLocation location;
};

const char* tokenTypeToString(TokenType type);

} // namespace nova
