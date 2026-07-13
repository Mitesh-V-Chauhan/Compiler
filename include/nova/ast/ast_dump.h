#pragma once
#include "nova/ast/visitor.h"
#include "nova/ast/ast_node.h"
#include <string>

namespace nova {

class ASTDumper : public ASTVisitor {
public:
    std::string dump(ASTNode& node);

    void visit(ProgramNode& node) override;
    
    void visit(BinaryExpression& node) override;
    void visit(UnaryExpression& node) override;
    void visit(LiteralNode& node) override;
    void visit(IdentifierNode& node) override;
    void visit(CallNode& node) override;
    void visit(ArrayNode& node) override;
    void visit(StructAccessNode& node) override;

    void visit(LetStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(BlockStmt& node) override;
    void visit(ExpressionStmt& node) override;
    void visit(BreakStmt& node) override;
    void visit(ContinueStmt& node) override;

    void visit(FunctionNode& node) override;
    void visit(StructNode& node) override;
    void visit(EnumNode& node) override;

private:
    std::string output_;
    int indent_level_ = 0;

    void indent();
};

} // namespace nova
