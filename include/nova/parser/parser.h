#pragma once
#include "nova/lexer/lexer.h"
#include "nova/ast/expressions.h"
#include "nova/ast/statements.h"
#include "nova/diagnostics/diagnostics.h"
#include <vector>
#include <memory>
#include <optional>

namespace nova {

class Parser {
public:
    Parser(std::vector<Token> tokens, Diagnostics& diagnostics);

    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens_;
    Diagnostics& diagnostics_;
    size_t current_ = 0;

    // Parsing helpers
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    bool check(TokenType type) const;
    bool match(std::initializer_list<TokenType> types);
    Token advance();
    Token consume(TokenType type, std::string_view message);
    
    void synchronize();
    void error(Token token, std::string_view message);

    // Declarations
    std::unique_ptr<ASTNode> declaration();
    std::unique_ptr<FunctionNode> functionDeclaration();
    std::unique_ptr<StructNode> structDeclaration();
    std::unique_ptr<EnumNode> enumDeclaration();

    // Statements
    std::unique_ptr<Statement> statement();
    std::unique_ptr<LetStmt> letStatement();
    std::unique_ptr<IfStmt> ifStatement();
    std::unique_ptr<WhileStmt> whileStatement();
    std::unique_ptr<ForStmt> forStatement();
    std::unique_ptr<ReturnStmt> returnStatement();
    std::unique_ptr<BreakStmt> breakStatement();
    std::unique_ptr<ContinueStmt> continueStatement();
    std::unique_ptr<BlockStmt> blockStatement();
    std::unique_ptr<ExpressionStmt> expressionStatement();

    // Expressions
    std::unique_ptr<Expression> expression();
    std::unique_ptr<Expression> assignment();
    std::unique_ptr<Expression> logicalOr();
    std::unique_ptr<Expression> logicalAnd();
    std::unique_ptr<Expression> equality();
    std::unique_ptr<Expression> comparison();
    std::unique_ptr<Expression> term();
    std::unique_ptr<Expression> factor();
    std::unique_ptr<Expression> unary();
    std::unique_ptr<Expression> callOrAccess();
    std::unique_ptr<Expression> primary();
    
    // Types
    std::optional<TypeIdentifier> parseType();
};

} // namespace nova
