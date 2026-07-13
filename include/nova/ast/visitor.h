#pragma once

namespace nova {

class ProgramNode;
class BinaryExpression;
class UnaryExpression;
class LiteralNode;
class IdentifierNode;
class CallNode;
class ArrayNode;
class StructAccessNode;

class LetStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BlockStmt;
class ExpressionStmt;
class BreakStmt;
class ContinueStmt;

class FunctionNode;
class StructNode;
class EnumNode;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(ProgramNode& node) = 0;
    
    // Expressions
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(LiteralNode& node) = 0;
    virtual void visit(IdentifierNode& node) = 0;
    virtual void visit(CallNode& node) = 0;
    virtual void visit(ArrayNode& node) = 0;
    virtual void visit(StructAccessNode& node) = 0;

    // Statements
    virtual void visit(LetStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(ExpressionStmt& node) = 0;
    virtual void visit(BreakStmt& node) = 0;
    virtual void visit(ContinueStmt& node) = 0;

    // Declarations
    virtual void visit(FunctionNode& node) = 0;
    virtual void visit(StructNode& node) = 0;
    virtual void visit(EnumNode& node) = 0;
};

} // namespace nova
