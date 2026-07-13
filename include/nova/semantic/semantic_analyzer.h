#pragma once
#include "nova/ast/visitor.h"
#include "nova/ast/expressions.h"
#include "nova/ast/statements.h"
#include "nova/semantic/symbol_table.h"
#include "nova/diagnostics/diagnostics.h"

namespace nova {

class SemanticAnalyzer : public ASTVisitor {
public:
    explicit SemanticAnalyzer(Diagnostics& diagnostics);
    ~SemanticAnalyzer();

    void analyze(ProgramNode& program);

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

    std::shared_ptr<Type> getNodeType(ASTNode* node);

private:
    Diagnostics& diagnostics_;
    Scope* current_scope_ = nullptr;
    std::unordered_map<ASTNode*, std::shared_ptr<Type>> node_types_;
    std::shared_ptr<Type> current_return_type_ = nullptr;
    
    std::unordered_map<std::string_view, std::shared_ptr<Type>> defined_types_;

    void enterScope();
    void leaveScope();
    std::shared_ptr<Type> resolveType(const TypeIdentifier& type_id, SourceLocation loc);
    std::shared_ptr<Type> checkBinaryOp(TokenType op, std::shared_ptr<Type> left, std::shared_ptr<Type> right, SourceLocation loc);
    
    void setNodeType(ASTNode* node, std::shared_ptr<Type> type);
};

} // namespace nova
