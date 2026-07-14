#include "nova/lexer/token.h"

namespace nova {

const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Int: return "Int";
        case TokenType::Float: return "Float";
        case TokenType::Bool: return "Bool";
        case TokenType::String: return "String";
        case TokenType::Char: return "Char";
        case TokenType::Let: return "Let";
        case TokenType::Fn: return "Fn";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::Break: return "Break";
        case TokenType::Return: return "Return";
        case TokenType::Extern: return "Extern";
        case TokenType::Struct: return "Struct";
        case TokenType::Enum: return "Enum";
        case TokenType::Module: return "Module";
        case TokenType::Import: return "Import";
        case TokenType::Identifier: return "Identifier";
        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::CharLiteral: return "CharLiteral";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::Ampersand: return "Ampersand";
        case TokenType::EqEq: return "EqEq";
        case TokenType::BangEq: return "BangEq";
        case TokenType::Greater: return "Greater";
        case TokenType::Less: return "Less";
        case TokenType::GreaterEq: return "GreaterEq";
        case TokenType::LessEq: return "LessEq";
        case TokenType::Eq: return "Eq";
        case TokenType::AndAnd: return "AndAnd";
        case TokenType::OrOr: return "OrOr";
        case TokenType::Bang: return "Bang";
        case TokenType::Arrow: return "Arrow";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::LBracket: return "LBracket";
        case TokenType::RBracket: return "RBracket";
        case TokenType::Comma: return "Comma";
        case TokenType::Colon: return "Colon";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Dot: return "Dot";
        case TokenType::Eof: return "Eof";
        case TokenType::Error: return "Error";
        default: return "Unknown";
    }
}

} // namespace nova
