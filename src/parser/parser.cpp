#include "nova/parser/parser.h"

namespace nova {

Parser::Parser(std::vector<Token> tokens, Diagnostics& diagnostics)
    : tokens_(std::move(tokens)), diagnostics_(diagnostics) {}

std::unique_ptr<ProgramNode> Parser::parse() {
    std::vector<std::unique_ptr<ASTNode>> declarations;
    while (!isAtEnd()) {
        try {
            declarations.push_back(declaration());
        } catch (const std::exception&) {
            synchronize();
        }
    }
    return std::make_unique<ProgramNode>(std::move(declarations));
}

// Helpers
bool Parser::isAtEnd() const {
    return peek().type == TokenType::Eof;
}

Token Parser::peek() const {
    return tokens_[current_];
}

Token Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

Token Parser::consume(TokenType type, std::string_view message) {
    if (check(type)) return advance();
    error(peek(), message);
    throw std::runtime_error("Parse error");
}

void Parser::error(Token token, std::string_view message) {
    diagnostics_.reportError(token.location, message);
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::Semicolon) return;
        switch (peek().type) {
            case TokenType::Let:
            case TokenType::Fn:
            case TokenType::If:
            case TokenType::While:
            case TokenType::For:
            case TokenType::Return:
            case TokenType::Struct:
            case TokenType::Enum:
                return;
            default:
                break;
        }
        advance();
    }
}

// Declarations
std::unique_ptr<ASTNode> Parser::declaration() {
    if (match({TokenType::Fn})) return functionDeclaration();
    if (match({TokenType::Struct})) return structDeclaration();
    if (match({TokenType::Enum})) return enumDeclaration();
    if (match({TokenType::Let})) return letStatement();
    return statement();
}

std::unique_ptr<FunctionNode> Parser::functionDeclaration() {
    Token name = consume(TokenType::Identifier, "Expected function name.");
    consume(TokenType::LParen, "Expected '(' after function name.");
    
    std::vector<FunctionNode::Param> params;
    if (!check(TokenType::RParen)) {
        do {
            Token paramName = consume(TokenType::Identifier, "Expected parameter name.");
            consume(TokenType::Colon, "Expected ':' after parameter name.");
            auto type = parseType();
            if (!type) {
                error(peek(), "Expected parameter type.");
                throw std::runtime_error("Parse error");
            }
            params.push_back({paramName, *type});
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RParen, "Expected ')' after parameters.");
    
    std::unique_ptr<TypeIdentifier> returnType;
    if (match({TokenType::Arrow})) {
        auto type = parseType();
        if (!type) {
            error(peek(), "Expected return type.");
            throw std::runtime_error("Parse error");
        }
        returnType = std::make_unique<TypeIdentifier>(*type);
    }
    
    consume(TokenType::LBrace, "Expected '{' before function body.");
    auto body = blockStatement();
    
    return std::make_unique<FunctionNode>(name, std::move(params), std::move(returnType), std::move(body));
}

std::unique_ptr<StructNode> Parser::structDeclaration() {
    Token name = consume(TokenType::Identifier, "Expected struct name.");
    consume(TokenType::LBrace, "Expected '{' before struct body.");
    
    std::vector<StructNode::Field> fields;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        Token fieldName = consume(TokenType::Identifier, "Expected field name.");
        consume(TokenType::Colon, "Expected ':' after field name.");
        auto type = parseType();
        if (!type) {
            error(peek(), "Expected field type.");
            throw std::runtime_error("Parse error");
        }
        fields.push_back({fieldName, *type});
        consume(TokenType::Semicolon, "Expected ';' after field declaration.");
    }
    consume(TokenType::RBrace, "Expected '}' after struct body.");
    
    return std::make_unique<StructNode>(name, std::move(fields));
}

std::unique_ptr<EnumNode> Parser::enumDeclaration() {
    Token name = consume(TokenType::Identifier, "Expected enum name.");
    consume(TokenType::LBrace, "Expected '{' before enum body.");
    
    std::vector<Token> variants;
    if (!check(TokenType::RBrace)) {
        do {
            variants.push_back(consume(TokenType::Identifier, "Expected enum variant."));
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RBrace, "Expected '}' after enum body.");
    
    return std::make_unique<EnumNode>(name, std::move(variants));
}

// Types
std::optional<TypeIdentifier> Parser::parseType() {
    if (match({TokenType::Int, TokenType::Float, TokenType::Bool, TokenType::String, TokenType::Char, TokenType::Identifier})) {
        return TypeIdentifier{previous().lexeme, false};
    }
    if (match({TokenType::LBracket})) {
        if (match({TokenType::Int, TokenType::Float, TokenType::Bool, TokenType::String, TokenType::Char, TokenType::Identifier})) {
            TypeIdentifier t = {previous().lexeme, true};
            consume(TokenType::RBracket, "Expected ']' after array type.");
            return t;
        }
    }
    return std::nullopt;
}

// Statements
std::unique_ptr<Statement> Parser::statement() {
    if (match({TokenType::If})) return ifStatement();
    if (match({TokenType::While})) return whileStatement();
    if (match({TokenType::For})) return forStatement();
    if (match({TokenType::Return})) return returnStatement();
    if (match({TokenType::Break})) return breakStatement();
    if (match({TokenType::Continue})) return continueStatement();
    if (match({TokenType::LBrace})) return blockStatement();
    return expressionStatement();
}

std::unique_ptr<LetStmt> Parser::letStatement() {
    Token name = consume(TokenType::Identifier, "Expected variable name.");
    
    std::unique_ptr<TypeIdentifier> type_id;
    if (match({TokenType::Colon})) {
        auto type = parseType();
        if (type) {
            type_id = std::make_unique<TypeIdentifier>(*type);
        } else {
            error(peek(), "Expected variable type.");
            throw std::runtime_error("Parse error");
        }
    }
    
    std::unique_ptr<Expression> initializer;
    if (match({TokenType::Eq})) {
        initializer = expression();
    }
    
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    return std::make_unique<LetStmt>(name, std::move(type_id), std::move(initializer));
}

std::unique_ptr<IfStmt> Parser::ifStatement() {
    SourceLocation loc = previous().location;
    consume(TokenType::LParen, "Expected '(' after 'if'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after if condition.");
    
    consume(TokenType::LBrace, "Expected '{' before if body.");
    auto thenBranch = blockStatement();
    
    std::unique_ptr<Statement> elseBranch;
    if (match({TokenType::Else})) {
        if (match({TokenType::If})) {
            elseBranch = ifStatement();
        } else {
            consume(TokenType::LBrace, "Expected '{' before else body.");
            elseBranch = blockStatement();
        }
    }
    
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), loc);
}

std::unique_ptr<WhileStmt> Parser::whileStatement() {
    SourceLocation loc = previous().location;
    consume(TokenType::LParen, "Expected '(' after 'while'.");
    auto condition = expression();
    consume(TokenType::RParen, "Expected ')' after while condition.");
    
    consume(TokenType::LBrace, "Expected '{' before while body.");
    auto body = blockStatement();
    
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), loc);
}

std::unique_ptr<ForStmt> Parser::forStatement() {
    SourceLocation loc = previous().location;
    consume(TokenType::LParen, "Expected '(' after 'for'.");
    
    std::unique_ptr<Statement> initializer;
    if (match({TokenType::Let})) {
        initializer = letStatement();
    } else if (!match({TokenType::Semicolon})) {
        initializer = expressionStatement();
    }
    
    std::unique_ptr<Expression> condition;
    if (!check(TokenType::Semicolon)) {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after loop condition.");
    
    std::unique_ptr<Expression> increment;
    if (!check(TokenType::RParen)) {
        increment = expression();
    }
    consume(TokenType::RParen, "Expected ')' after for clauses.");
    
    consume(TokenType::LBrace, "Expected '{' before for body.");
    auto body = blockStatement();
    
    return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body), loc);
}

std::unique_ptr<ReturnStmt> Parser::returnStatement() {
    Token keyword = previous();
    std::unique_ptr<Expression> value;
    if (!check(TokenType::Semicolon)) {
        value = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after return value.");
    return std::make_unique<ReturnStmt>(keyword, std::move(value));
}

std::unique_ptr<BreakStmt> Parser::breakStatement() {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expected ';' after break.");
    return std::make_unique<BreakStmt>(keyword);
}

std::unique_ptr<ContinueStmt> Parser::continueStatement() {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expected ';' after continue.");
    return std::make_unique<ContinueStmt>(keyword);
}

std::unique_ptr<BlockStmt> Parser::blockStatement() {
    SourceLocation loc = previous().location;
    std::vector<std::unique_ptr<Statement>> statements;
    while (!check(TokenType::RBrace) && !isAtEnd()) {
        try {
            if (match({TokenType::Let})) {
                statements.push_back(letStatement());
            } else {
                statements.push_back(statement());
            }
        } catch (const std::exception&) {
            synchronize();
        }
    }
    consume(TokenType::RBrace, "Expected '}' after block.");
    return std::make_unique<BlockStmt>(std::move(statements), loc);
}

std::unique_ptr<ExpressionStmt> Parser::expressionStatement() {
    auto expr = expression();
    consume(TokenType::Semicolon, "Expected ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

// Expressions
std::unique_ptr<Expression> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expression> Parser::assignment() {
    auto expr = logicalOr();
    
    if (match({TokenType::Eq})) {
        Token equals = previous();
        auto value = assignment();
        return std::make_unique<BinaryExpression>(std::move(expr), equals, std::move(value));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (match({TokenType::OrOr})) {
        Token op = previous();
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::logicalAnd() {
    auto expr = equality();
    while (match({TokenType::AndAnd})) {
        Token op = previous();
        auto right = equality();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::equality() {
    auto expr = comparison();
    while (match({TokenType::EqEq, TokenType::BangEq})) {
        Token op = previous();
        auto right = comparison();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::comparison() {
    auto expr = term();
    while (match({TokenType::Greater, TokenType::GreaterEq, TokenType::Less, TokenType::LessEq})) {
        Token op = previous();
        auto right = term();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::term() {
    auto expr = factor();
    while (match({TokenType::Plus, TokenType::Minus})) {
        Token op = previous();
        auto right = factor();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::factor() {
    auto expr = unary();
    while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
        Token op = previous();
        auto right = unary();
        expr = std::make_unique<BinaryExpression>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expression> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus})) {
        Token op = previous();
        auto right = unary();
        return std::make_unique<UnaryExpression>(op, std::move(right));
    }
    return callOrAccess();
}

std::unique_ptr<Expression> Parser::callOrAccess() {
    auto expr = primary();
    
    while (true) {
        if (match({TokenType::LParen})) {
            std::vector<std::unique_ptr<Expression>> args;
            if (!check(TokenType::RParen)) {
                do {
                    args.push_back(expression());
                } while (match({TokenType::Comma}));
            }
            consume(TokenType::RParen, "Expected ')' after arguments.");
            expr = std::make_unique<CallNode>(std::move(expr), std::move(args));
        } else if (match({TokenType::Dot})) {
            Token name = consume(TokenType::Identifier, "Expected property name after '.'.");
            expr = std::make_unique<StructAccessNode>(std::move(expr), name);
        } else {
            break;
        }
    }
    
    return expr;
}

std::unique_ptr<Expression> Parser::primary() {
    if (match({TokenType::False, TokenType::True, TokenType::IntegerLiteral, TokenType::FloatLiteral, TokenType::StringLiteral, TokenType::CharLiteral})) {
        return std::make_unique<LiteralNode>(previous());
    }
    
    if (match({TokenType::Identifier})) {
        return std::make_unique<IdentifierNode>(previous());
    }
    
    if (match({TokenType::LParen})) {
        auto expr = expression();
        consume(TokenType::RParen, "Expected ')' after expression.");
        return expr;
    }
    
    if (match({TokenType::LBracket})) {
        SourceLocation loc = previous().location;
        std::vector<std::unique_ptr<Expression>> elements;
        if (!check(TokenType::RBracket)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RBracket, "Expected ']' after array elements.");
        return std::make_unique<ArrayNode>(std::move(elements), loc);
    }
    
    error(peek(), "Expected expression.");
    throw std::runtime_error("Parse error");
}

} // namespace nova
