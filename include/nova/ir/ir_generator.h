#pragma once
#include "nova/ast/visitor.h"
#include "nova/ir/ir.h"
#include "nova/ast/expressions.h"
#include "nova/ast/statements.h"
#include <string>

namespace nova {

class IRGenerator : public ASTVisitor {
public:
    IRProgram generate(ProgramNode& program);
    
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
    IRProgram program_;
    IRFunction* current_func_ = nullptr;
    BasicBlock* current_block_ = nullptr;
    
    int temp_counter_ = 0;
    int label_counter_ = 0;
    
    std::string current_loop_start_;
    std::string current_loop_end_;
    
    IRValue last_value_;
    
    IRValue newTemp();
    std::string newLabel(const std::string& prefix = "L");
    void emit(IROp op, IRValue dest, IRValue arg1 = {}, IRValue arg2 = {});
    void emitJump(const std::string& label);
    void emitBranch(IRValue cond, const std::string& true_label, const std::string& false_label);
    void enterBlock(const std::string& label);
};

} // namespace nova
