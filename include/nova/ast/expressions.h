#pragma once
#include "nova/ast/ast_node.h"
#include <memory>
#include <vector>

namespace nova {

class LiteralNode : public Expression {
public:
    Token token;
    
    explicit LiteralNode(Token t) : token(t) { location = t.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class IdentifierNode : public Expression {
public:
    Token token;
    
    explicit IdentifierNode(Token t) : token(t) { location = t.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class BinaryExpression : public Expression {
public:
    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
    
    BinaryExpression(std::unique_ptr<Expression> l, Token o, std::unique_ptr<Expression> r)
        : left(std::move(l)), op(o), right(std::move(r)) { location = left->location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class UnaryExpression : public Expression {
public:
    Token op;
    std::unique_ptr<Expression> operand;
    
    UnaryExpression(Token o, std::unique_ptr<Expression> r)
        : op(o), operand(std::move(r)) { location = op.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class CallNode : public Expression {
public:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    
    CallNode(std::unique_ptr<Expression> c, std::vector<std::unique_ptr<Expression>> args)
        : callee(std::move(c)), arguments(std::move(args)) { location = callee->location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ArrayNode : public Expression {
public:
    std::vector<std::unique_ptr<Expression>> elements;
    
    explicit ArrayNode(std::vector<std::unique_ptr<Expression>> elems, SourceLocation loc)
        : elements(std::move(elems)) { location = loc; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class StructAccessNode : public Expression {
public:
    std::unique_ptr<Expression> object;
    Token field;
    
    StructAccessNode(std::unique_ptr<Expression> obj, Token f)
        : object(std::move(obj)), field(f) { location = object->location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

} // namespace nova
