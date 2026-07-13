#include "nova/ir/ir_generator.h"
#include <iostream>

namespace nova {

IRProgram IRGenerator::generate(ProgramNode& program) {
    program.accept(*this);
    return std::move(program_);
}

IRValue IRGenerator::newTemp() {
    return {IRValue::Type::Temp, "t" + std::to_string(temp_counter_++)};
}

std::string IRGenerator::newLabel(const std::string& prefix) {
    return prefix + std::to_string(label_counter_++);
}

void IRGenerator::emit(IROp op, IRValue dest, IRValue arg1, IRValue arg2) {
    if (current_block_) {
        current_block_->instructions.push_back({op, dest, arg1, arg2});
    }
}

void IRGenerator::emitJump(const std::string& label) {
    emit(IROp::Jump, {IRValue::Type::Label, label});
}

void IRGenerator::emitBranch(IRValue cond, const std::string& true_label, const std::string& false_label) {
    emit(IROp::Branch, {IRValue::Type::Label, true_label}, cond, {IRValue::Type::Label, false_label});
}

void IRGenerator::enterBlock(const std::string& label) {
    if (!current_func_) return;
    current_func_->blocks.push_back({label, {}});
    current_block_ = &current_func_->blocks.back();
}

void IRGenerator::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void IRGenerator::visit(FunctionNode& node) {
    IRFunction func;
    func.name = std::string(node.name.lexeme);
    program_.functions.push_back(std::move(func));
    current_func_ = &program_.functions.back();
    
    enterBlock(newLabel("entry"));
    
    node.body->accept(*this);
    
    current_func_ = nullptr;
    current_block_ = nullptr;
}

void IRGenerator::visit(BlockStmt& node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void IRGenerator::visit(LetStmt& node) {
    IRValue var = {IRValue::Type::Variable, std::string(node.identifier.lexeme)};
    emit(IROp::Alloc, var);
    if (node.initializer) {
        node.initializer->accept(*this);
        emit(IROp::Assign, var, last_value_);
    }
}

void IRGenerator::visit(ExpressionStmt& node) {
    node.expression->accept(*this);
}

void IRGenerator::visit(ReturnStmt& node) {
    if (node.value) {
        node.value->accept(*this);
        emit(IROp::Return, {}, last_value_);
    } else {
        emit(IROp::Return, {}, {IRValue::Type::None, ""});
    }
}

void IRGenerator::visit(IfStmt& node) {
    node.condition->accept(*this);
    IRValue cond = last_value_;
    
    std::string then_lbl = newLabel("then");
    std::string else_lbl = newLabel("else");
    std::string end_lbl = newLabel("endif");
    
    emitBranch(cond, then_lbl, node.else_branch ? else_lbl : end_lbl);
    
    enterBlock(then_lbl);
    node.then_branch->accept(*this);
    emitJump(end_lbl);
    
    if (node.else_branch) {
        enterBlock(else_lbl);
        node.else_branch->accept(*this);
        emitJump(end_lbl);
    }
    
    enterBlock(end_lbl);
}

void IRGenerator::visit(WhileStmt& node) {
    std::string cond_lbl = newLabel("while_cond");
    std::string body_lbl = newLabel("while_body");
    std::string end_lbl = newLabel("while_end");
    
    std::string old_start = current_loop_start_;
    std::string old_end = current_loop_end_;
    current_loop_start_ = cond_lbl;
    current_loop_end_ = end_lbl;
    
    emitJump(cond_lbl);
    enterBlock(cond_lbl);
    
    node.condition->accept(*this);
    emitBranch(last_value_, body_lbl, end_lbl);
    
    enterBlock(body_lbl);
    node.body->accept(*this);
    emitJump(cond_lbl);
    
    enterBlock(end_lbl);
    
    current_loop_start_ = old_start;
    current_loop_end_ = old_end;
}

void IRGenerator::visit(ForStmt& node) {
    if (node.initializer) node.initializer->accept(*this);
    
    std::string cond_lbl = newLabel("for_cond");
    std::string body_lbl = newLabel("for_body");
    std::string inc_lbl = newLabel("for_inc");
    std::string end_lbl = newLabel("for_end");
    
    std::string old_start = current_loop_start_;
    std::string old_end = current_loop_end_;
    current_loop_start_ = inc_lbl;
    current_loop_end_ = end_lbl;
    
    emitJump(cond_lbl);
    enterBlock(cond_lbl);
    
    if (node.condition) {
        node.condition->accept(*this);
        emitBranch(last_value_, body_lbl, end_lbl);
    } else {
        emitJump(body_lbl);
    }
    
    enterBlock(body_lbl);
    node.body->accept(*this);
    emitJump(inc_lbl);
    
    enterBlock(inc_lbl);
    if (node.increment) node.increment->accept(*this);
    emitJump(cond_lbl);
    
    enterBlock(end_lbl);
    
    current_loop_start_ = old_start;
    current_loop_end_ = old_end;
}

void IRGenerator::visit(BreakStmt& node) {
    emitJump(current_loop_end_);
}

void IRGenerator::visit(ContinueStmt& node) {
    emitJump(current_loop_start_);
}

void IRGenerator::visit(BinaryExpression& node) {
    node.left->accept(*this);
    IRValue left = last_value_;
    
    node.right->accept(*this);
    IRValue right = last_value_;
    
    if (node.op.type == TokenType::Eq) {
        emit(IROp::Assign, left, right);
        last_value_ = left;
        return;
    }
    
    IROp op = IROp::Add;
    switch (node.op.type) {
        case TokenType::Plus: op = IROp::Add; break;
        case TokenType::Minus: op = IROp::Sub; break;
        case TokenType::Star: op = IROp::Mul; break;
        case TokenType::Slash: op = IROp::Div; break;
        case TokenType::Percent: op = IROp::Mod; break;
        case TokenType::EqEq: op = IROp::Eq; break;
        case TokenType::BangEq: op = IROp::Neq; break;
        case TokenType::Less: op = IROp::Lt; break;
        case TokenType::LessEq: op = IROp::Lte; break;
        case TokenType::Greater: op = IROp::Gt; break;
        case TokenType::GreaterEq: op = IROp::Gte; break;
        case TokenType::AndAnd: op = IROp::And; break;
        case TokenType::OrOr: op = IROp::Or; break;
        default: break;
    }
    
    IRValue dest = newTemp();
    emit(op, dest, left, right);
    last_value_ = dest;
}

void IRGenerator::visit(UnaryExpression& node) {
    node.operand->accept(*this);
    IRValue operand = last_value_;
    
    IROp op = IROp::Neg;
    if (node.op.type == TokenType::Bang) op = IROp::Not;
    
    IRValue dest = newTemp();
    emit(op, dest, operand);
    last_value_ = dest;
}

void IRGenerator::visit(LiteralNode& node) {
    last_value_ = {IRValue::Type::Constant, std::string(node.token.lexeme)};
}

void IRGenerator::visit(IdentifierNode& node) {
    last_value_ = {IRValue::Type::Variable, std::string(node.token.lexeme)};
}

void IRGenerator::visit(CallNode& node) {
    for (auto& arg : node.arguments) {
        arg->accept(*this);
        emit(IROp::Param, {}, last_value_);
    }
    node.callee->accept(*this);
    IRValue dest = newTemp();
    emit(IROp::Call, dest, last_value_);
    last_value_ = dest;
}

// Stubs for now
void IRGenerator::visit(ArrayNode& node) {}
void IRGenerator::visit(StructAccessNode& node) {}
void IRGenerator::visit(StructNode& node) {}
void IRGenerator::visit(EnumNode& node) {}

} // namespace nova
