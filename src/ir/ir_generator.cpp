#include "nova/ir/ir_generator.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/backend/memory_layout.h"
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
        if (auto* let = dynamic_cast<LetStmt*>(decl.get())) {
            std::string name(let->identifier.lexeme);
            global_names_.insert(name);
            
            IRGlobalVar gvar;
            gvar.name = name;
            gvar.size = 8;
            gvar.has_init = false;
            gvar.init_val = 0;
            
            if (let->initializer) {
                if (auto* lit = dynamic_cast<LiteralNode*>(let->initializer.get())) {
                    if (lit->token.type == TokenType::IntegerLiteral) {
                        gvar.init_val = std::stoll(std::string(lit->token.lexeme));
                        gvar.has_init = true;
                    } else if (lit->token.type == TokenType::True) {
                        gvar.init_val = 1;
                        gvar.has_init = true;
                    } else if (lit->token.type == TokenType::False) {
                        gvar.init_val = 0;
                        gvar.has_init = true;
                    }
                }
            }
            program_.globals.push_back(gvar);
        }
    }

    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void IRGenerator::visit(FunctionNode& node) {
    if (node.is_extern) return;
    
    IRFunction func;
    func.name = std::string(node.name.lexeme);
    program_.functions.push_back(std::move(func));
    current_func_ = &program_.functions.back();
    
    enterBlock(newLabel("entry"));
    
    for (size_t i = 0; i < node.params.size(); ++i) {
        IRValue dest{IRValue::Type::Variable, std::string(node.params[i].name.lexeme)};
        int size = 8;
        emit(IROp::Alloc, dest, {IRValue::Type::Constant, std::to_string(size)});
        emit(IROp::GetParam, dest, {IRValue::Type::Constant, std::to_string(i)});
    }
    
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
    if (!current_func_) {
        // Global variables are collected and initialized at the program level
        return;
    }

    if (node.initializer) {
        node.initializer->accept(*this);
        IRValue init_val = last_value_;
        
        IRValue dest{IRValue::Type::Variable, std::string(node.identifier.lexeme)};
        
        int size = 8;
        if (semantic_) {
            auto type = semantic_->getNodeType(node.initializer.get());
            size = MemoryLayout::getSize(type);
        }
        
        emit(IROp::Alloc, dest, {IRValue::Type::Constant, std::to_string(size)});
        emit(IROp::Assign, dest, init_val);
    } else {
        IRValue dest{IRValue::Type::Variable, std::string(node.identifier.lexeme)};
        emit(IROp::Alloc, dest, {IRValue::Type::Constant, "8"});
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
    if (node.op.type == TokenType::Eq) {
        bool was_lvalue = lvalue_mode_;
        lvalue_mode_ = true;
        node.left->accept(*this);
        IRValue left = last_value_;
        lvalue_mode_ = was_lvalue;
        
        node.right->accept(*this);
        IRValue right = last_value_;
        
        if (dynamic_cast<StructAccessNode*>(node.left.get())) {
            emit(IROp::Store, {}, left, right); // Store right into address left
        } else if (dynamic_cast<IndexAccessNode*>(node.left.get())) {
            emit(IROp::Store, {}, left, right); // Store right into address left
        } else if (auto unary = dynamic_cast<UnaryExpression*>(node.left.get()); unary && unary->op.type == TokenType::Star) {
            emit(IROp::Store, {}, left, right); // Store right into pointer left
        } else {
            emit(IROp::Assign, left, right);
        }
        last_value_ = left;
        return;
    }
    
    node.left->accept(*this);
    IRValue left = last_value_;
    
    node.right->accept(*this);
    IRValue right = last_value_;
    
    if (node.op.type == TokenType::Plus || node.op.type == TokenType::Minus) {
        bool left_is_ptr = false;
        bool right_is_ptr = false;
        int elem_size = 8;
        if (semantic_) {
            auto lt = semantic_->getNodeType(node.left.get());
            auto rt = semantic_->getNodeType(node.right.get());
            if (lt && lt->pointer_depth > 0) left_is_ptr = true;
            if (rt && rt->pointer_depth > 0) right_is_ptr = true;
            if (left_is_ptr && lt) {
                if (lt->pointer_depth > 1) elem_size = 8;
                else {
                    auto deref = std::make_shared<Type>(lt->kind, lt->name);
                    elem_size = MemoryLayout::getSize(deref);
                }
            } else if (right_is_ptr && rt) {
                if (rt->pointer_depth > 1) elem_size = 8;
                else {
                    auto deref = std::make_shared<Type>(rt->kind, rt->name);
                    elem_size = MemoryLayout::getSize(deref);
                }
            }
        }
        if (elem_size <= 0) elem_size = 8;

        if (left_is_ptr && !right_is_ptr) {
            IRValue offset = newTemp();
            emit(IROp::Mul, offset, right, {IRValue::Type::Constant, std::to_string(elem_size)});
            IRValue dest = newTemp();
            emit(node.op.type == TokenType::Plus ? IROp::Add : IROp::Sub, dest, left, offset);
            last_value_ = dest;
            return;
        } else if (right_is_ptr && !left_is_ptr && node.op.type == TokenType::Plus) {
            IRValue offset = newTemp();
            emit(IROp::Mul, offset, left, {IRValue::Type::Constant, std::to_string(elem_size)});
            IRValue dest = newTemp();
            emit(IROp::Add, dest, right, offset);
            last_value_ = dest;
            return;
        }
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
    if (node.op.type == TokenType::Ampersand) {
        node.operand->accept(*this);
        IRValue dest = newTemp();
        emit(IROp::AddressOf, dest, last_value_);
        last_value_ = dest;
        return;
    }
    
    if (node.op.type == TokenType::Star) {
        node.operand->accept(*this);
        IRValue ptr = last_value_;
        if (lvalue_mode_) {
            last_value_ = ptr;
        } else {
            IRValue val = newTemp();
            emit(IROp::Load, val, ptr);
            last_value_ = val;
        }
        return;
    }

    node.operand->accept(*this);
    IRValue operand = last_value_;
    
    IROp op = IROp::Neg;
    if (node.op.type == TokenType::Bang) op = IROp::Not;
    
    IRValue dest = newTemp();
    emit(op, dest, operand);
    last_value_ = dest;
}

void IRGenerator::visit(LiteralNode& node) {
    if (node.token.type == TokenType::StringLiteral) {
        std::string val = std::string(node.token.lexeme);
        if (val.size() >= 2) val = val.substr(1, val.size() - 2);
        
        std::string unescaped;
        for (size_t i = 0; i < val.size(); ++i) {
            if (val[i] == '\\' && i + 1 < val.size()) {
                char next = val[i+1];
                if (next == 'n') unescaped += '\n';
                else if (next == 't') unescaped += '\t';
                else if (next == 'r') unescaped += '\r';
                else if (next == '\\') unescaped += '\\';
                else if (next == '"') unescaped += '"';
                else unescaped += val[i];
                i++;
            } else {
                unescaped += val[i];
            }
        }
        val = unescaped;
        
        std::string label = ".L.str" + std::to_string(program_.strings.size());
        program_.strings.push_back({label, val});
        
        last_value_ = {IRValue::Type::Label, label};
    } else {
        last_value_ = {IRValue::Type::Constant, std::string(node.token.lexeme)};
    }
}

void IRGenerator::visit(IdentifierNode& node) {
    std::string name(node.token.lexeme);
    if (global_names_.find(name) != global_names_.end()) {
        last_value_ = {IRValue::Type::Global, name};
    } else {
        last_value_ = {IRValue::Type::Variable, name};
    }
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
void IRGenerator::visit(ArrayNode& node) {
    // Array literals not fully implemented for codegen yet, just return a temp
    last_value_ = newTemp();
}

void IRGenerator::visit(StructAccessNode& node) {
    node.object->accept(*this);
    IRValue base = last_value_;
    
    int offset = 0;
    if (semantic_) {
        auto obj_type = semantic_->getNodeType(node.object.get());
        if (obj_type && obj_type->kind == TypeKind::Struct) {
            auto layout = MemoryLayout::calculateStructLayout(obj_type->struct_fields);
            offset = layout.field_offsets[std::string(node.field.lexeme)];
        }
    }
    
    IRValue addr = newTemp();
    emit(IROp::GetElementPtr, addr, base, {IRValue::Type::Constant, std::to_string(offset)});
    
    if (lvalue_mode_) {
        last_value_ = addr;
    } else {
        IRValue val = newTemp();
        emit(IROp::Load, val, addr);
        last_value_ = val;
    }
}

void IRGenerator::visit(IndexAccessNode& node) {
    node.target->accept(*this);
    IRValue base = last_value_;
    
    node.index->accept(*this);
    IRValue idx = last_value_;
    
    int elem_size = 8;
    bool is_stack_array = false;
    if (semantic_) {
        auto target_type = semantic_->getNodeType(node.target.get());
        if (target_type) {
            if (target_type->kind == TypeKind::Array) {
                is_stack_array = true;
                if (target_type->element_type) {
                    elem_size = MemoryLayout::getSize(target_type->element_type);
                }
            } else if (target_type->pointer_depth > 1) {
                elem_size = 8;
            } else if (target_type->pointer_depth == 1) {
                auto deref = std::make_shared<Type>(target_type->kind, target_type->name);
                elem_size = MemoryLayout::getSize(deref);
            }
        }
    }
    if (elem_size <= 0) elem_size = 8;
    
    IRValue scaled_offset = newTemp();
    emit(IROp::Mul, scaled_offset, idx, {IRValue::Type::Constant, std::to_string(elem_size)});
    
    IRValue addr = newTemp();
    if (is_stack_array) {
        emit(IROp::GetElementPtr, addr, base, scaled_offset);
    } else {
        // Pointer / heap buffer: base is already a memory address
        emit(IROp::Add, addr, base, scaled_offset);
    }
    
    if (lvalue_mode_) {
        last_value_ = addr;
    } else {
        IRValue val = newTemp();
        emit(IROp::Load, val, addr);
        last_value_ = val;
    }
}

void IRGenerator::visit(StructNode& node) {}
void IRGenerator::visit(EnumNode& node) {}

} // namespace nova
