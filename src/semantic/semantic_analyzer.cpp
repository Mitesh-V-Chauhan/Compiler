#include "nova/semantic/semantic_analyzer.h"
#include <iostream>

namespace nova {

SemanticAnalyzer::SemanticAnalyzer(Diagnostics& diagnostics) : diagnostics_(diagnostics) {
    // Add built-in types
    defined_types_["int"] = Type::getInt();
    defined_types_["float"] = Type::getFloat();
    defined_types_["bool"] = Type::getBool();
    defined_types_["string"] = Type::getString();
    defined_types_["char"] = Type::getChar();
    defined_types_["void"] = Type::getVoid();
}

SemanticAnalyzer::~SemanticAnalyzer() {
    while (current_scope_) {
        leaveScope();
    }
}

void SemanticAnalyzer::enterScope() {
    current_scope_ = new Scope(current_scope_);
}

void SemanticAnalyzer::leaveScope() {
    Scope* old = current_scope_;
    current_scope_ = current_scope_->parent;
    delete old;
}

void SemanticAnalyzer::setNodeType(ASTNode* node, std::shared_ptr<Type> type) {
    node_types_[node] = type;
}

std::shared_ptr<Type> SemanticAnalyzer::getNodeType(ASTNode* node) {
    auto it = node_types_.find(node);
    if (it != node_types_.end()) return it->second;
    return Type::getUnknown();
}

void SemanticAnalyzer::analyze(ProgramNode& program) {
    enterScope(); // Global scope
    
    // Pass 1: Register all top-level types (Structs, Enums) and Functions
    for (auto& decl : program.declarations) {
        if (auto* func = dynamic_cast<FunctionNode*>(decl.get())) {
            auto func_type = std::make_shared<Type>(TypeKind::Function, std::string(func->name.lexeme));
            
            if (func->return_type) {
                func_type->return_type = resolveType(*func->return_type, func->name.location);
            } else {
                func_type->return_type = Type::getVoid();
            }
            
            for (auto& p : func->params) {
                func_type->param_types.push_back(resolveType(p.type, p.name.location));
            }
            
            if (!current_scope_->define({func->name.lexeme, func_type, false})) {
                diagnostics_.reportError(func->name.location, "Duplicate function declaration: " + std::string(func->name.lexeme));
            }
        } else if (auto* struct_decl = dynamic_cast<StructNode*>(decl.get())) {
            auto struct_type = std::make_shared<Type>(TypeKind::Struct, std::string(struct_decl->name.lexeme));
            defined_types_[struct_decl->name.lexeme] = struct_type;
        }
    }
    
    // Pass 2: Actually visit and analyze everything
    program.accept(*this);
    
    leaveScope();
}

void SemanticAnalyzer::visit(ProgramNode& node) {
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void SemanticAnalyzer::visit(FunctionNode& node) {
    Symbol* sym = current_scope_->lookup(node.name.lexeme);
    if (!sym) return; // Should be handled in pass 1
    
    enterScope();
    current_return_type_ = sym->type->return_type;
    
    for (size_t i = 0; i < node.params.size(); ++i) {
        if (!current_scope_->define({node.params[i].name.lexeme, sym->type->param_types[i], false})) {
            diagnostics_.reportError(node.params[i].name.location, "Duplicate parameter name: " + std::string(node.params[i].name.lexeme));
        }
    }
    
    node.body->accept(*this);
    
    current_return_type_ = nullptr;
    leaveScope();
}

void SemanticAnalyzer::visit(BlockStmt& node) {
    enterScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    leaveScope();
}

void SemanticAnalyzer::visit(LetStmt& node) {
    std::shared_ptr<Type> type = nullptr;
    
    if (node.initializer) {
        node.initializer->accept(*this);
        type = getNodeType(node.initializer.get());
    }
    
    if (node.type_id) {
        auto declared_type = resolveType(*node.type_id, node.identifier.location);
        if (type && type->kind != TypeKind::Unknown && declared_type->kind != TypeKind::Unknown && *type != *declared_type) {
            diagnostics_.reportError(node.initializer->location, "Type mismatch in initialization.");
        }
        type = declared_type;
    }
    
    if (!type) {
        diagnostics_.reportError(node.identifier.location, "Cannot infer type for variable.");
        type = Type::getUnknown();
    }
    
    if (!current_scope_->define({node.identifier.lexeme, type, true})) {
        diagnostics_.reportError(node.identifier.location, "Duplicate variable declaration.");
    }
}

void SemanticAnalyzer::visit(IdentifierNode& node) {
    Symbol* sym = current_scope_->lookup(node.token.lexeme);
    if (!sym) {
        diagnostics_.reportError(node.token.location, "Undefined variable: " + std::string(node.token.lexeme));
        setNodeType(&node, Type::getUnknown());
    } else {
        setNodeType(&node, sym->type);
    }
}

void SemanticAnalyzer::visit(LiteralNode& node) {
    switch (node.token.type) {
        case TokenType::IntegerLiteral: setNodeType(&node, Type::getInt()); break;
        case TokenType::FloatLiteral: setNodeType(&node, Type::getFloat()); break;
        case TokenType::True:
        case TokenType::False: setNodeType(&node, Type::getBool()); break;
        case TokenType::StringLiteral: setNodeType(&node, Type::getString()); break;
        case TokenType::CharLiteral: setNodeType(&node, Type::getChar()); break;
        default: setNodeType(&node, Type::getUnknown()); break;
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node) {
    node.left->accept(*this);
    node.right->accept(*this);
    
    auto left_type = getNodeType(node.left.get());
    auto right_type = getNodeType(node.right.get());
    
    if (node.op.type == TokenType::Eq) {
        // Assignment check
        if (*left_type != *right_type && left_type->kind != TypeKind::Unknown && right_type->kind != TypeKind::Unknown) {
            diagnostics_.reportError(node.op.location, "Cannot assign value of different type.");
        }
        setNodeType(&node, left_type);
        return;
    }
    
    auto res = checkBinaryOp(node.op.type, left_type, right_type, node.op.location);
    setNodeType(&node, res);
}

std::shared_ptr<Type> SemanticAnalyzer::checkBinaryOp(TokenType op, std::shared_ptr<Type> left, std::shared_ptr<Type> right, SourceLocation loc) {
    if (left->kind == TypeKind::Unknown || right->kind == TypeKind::Unknown) return Type::getUnknown();
    
    switch (op) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            if (left->kind != TypeKind::Int && left->kind != TypeKind::Float) {
                diagnostics_.reportError(loc, "Arithmetic operations require numeric types.");
                return Type::getUnknown();
            }
            if (*left != *right) {
                diagnostics_.reportError(loc, "Type mismatch in arithmetic operation.");
                return Type::getUnknown();
            }
            return left;
            
        case TokenType::EqEq:
        case TokenType::BangEq:
            if (*left != *right) {
                diagnostics_.reportError(loc, "Type mismatch in equality comparison.");
            }
            return Type::getBool();
            
        case TokenType::Less:
        case TokenType::LessEq:
        case TokenType::Greater:
        case TokenType::GreaterEq:
            if (left->kind != TypeKind::Int && left->kind != TypeKind::Float) {
                diagnostics_.reportError(loc, "Comparisons require numeric types.");
                return Type::getBool();
            }
            if (*left != *right) {
                diagnostics_.reportError(loc, "Type mismatch in comparison.");
            }
            return Type::getBool();
            
        case TokenType::AndAnd:
        case TokenType::OrOr:
            if (left->kind != TypeKind::Bool || right->kind != TypeKind::Bool) {
                diagnostics_.reportError(loc, "Logical operations require boolean types.");
            }
            return Type::getBool();
            
        default:
            return Type::getUnknown();
    }
}

void SemanticAnalyzer::visit(UnaryExpression& node) {
    node.operand->accept(*this);
    auto type = getNodeType(node.operand.get());
    
    if (node.op.type == TokenType::Minus) {
        if (type->kind != TypeKind::Int && type->kind != TypeKind::Float && type->kind != TypeKind::Unknown) {
            diagnostics_.reportError(node.op.location, "Unary minus requires numeric type.");
        }
        setNodeType(&node, type);
    } else if (node.op.type == TokenType::Bang) {
        if (type->kind != TypeKind::Bool && type->kind != TypeKind::Unknown) {
            diagnostics_.reportError(node.op.location, "Logical NOT requires boolean type.");
        }
        setNodeType(&node, Type::getBool());
    } else if (node.op.type == TokenType::Ampersand) {
        auto ptr_type = std::make_shared<Type>(type->kind, type->name);
        ptr_type->pointer_depth = type->pointer_depth + 1;
        setNodeType(&node, ptr_type);
    } else if (node.op.type == TokenType::Star) {
        if (type->pointer_depth == 0) {
            diagnostics_.reportError(node.op.location, "Cannot dereference non-pointer type.");
            setNodeType(&node, Type::getUnknown());
        } else {
            auto deref_type = std::make_shared<Type>(type->kind, type->name);
            deref_type->pointer_depth = type->pointer_depth - 1;
            setNodeType(&node, deref_type);
        }
    } else {
        setNodeType(&node, Type::getUnknown());
    }
}

void SemanticAnalyzer::visit(CallNode& node) {
    node.callee->accept(*this);
    auto callee_type = getNodeType(node.callee.get());
    
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
    
    if (callee_type->kind != TypeKind::Function) {
        if (callee_type->kind != TypeKind::Unknown) {
            diagnostics_.reportError(node.location, "Attempted to call a non-function type.");
        }
        setNodeType(&node, Type::getUnknown());
        return;
    }
    
    if (node.arguments.size() != callee_type->param_types.size()) {
        diagnostics_.reportError(node.location, "Argument count mismatch.");
    } else {
        for (size_t i = 0; i < node.arguments.size(); ++i) {
            auto arg_type = getNodeType(node.arguments[i].get());
            if (*arg_type != *callee_type->param_types[i] && arg_type->kind != TypeKind::Unknown && callee_type->param_types[i]->kind != TypeKind::Unknown) {
                diagnostics_.reportError(node.arguments[i]->location, "Argument type mismatch.");
            }
        }
    }
    
    setNodeType(&node, callee_type->return_type);
}

void SemanticAnalyzer::visit(ReturnStmt& node) {
    if (node.value) {
        node.value->accept(*this);
        auto val_type = getNodeType(node.value.get());
        
        if (current_return_type_ && current_return_type_->kind != TypeKind::Unknown && val_type->kind != TypeKind::Unknown && *current_return_type_ != *val_type) {
            diagnostics_.reportError(node.location, "Return type mismatch.");
        }
    } else {
        if (current_return_type_ && current_return_type_->kind != TypeKind::Void) {
            diagnostics_.reportError(node.location, "Expected return value.");
        }
    }
}

void SemanticAnalyzer::visit(IfStmt& node) {
    node.condition->accept(*this);
    auto cond_type = getNodeType(node.condition.get());
    if (cond_type->kind != TypeKind::Bool && cond_type->kind != TypeKind::Unknown) {
        diagnostics_.reportError(node.condition->location, "Condition must be boolean.");
    }
    
    node.then_branch->accept(*this);
    if (node.else_branch) {
        node.else_branch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    node.condition->accept(*this);
    auto cond_type = getNodeType(node.condition.get());
    if (cond_type->kind != TypeKind::Bool && cond_type->kind != TypeKind::Unknown) {
        diagnostics_.reportError(node.condition->location, "Condition must be boolean.");
    }
    
    node.body->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt& node) {
    enterScope();
    if (node.initializer) node.initializer->accept(*this);
    if (node.condition) {
        node.condition->accept(*this);
        auto cond_type = getNodeType(node.condition.get());
        if (cond_type->kind != TypeKind::Bool && cond_type->kind != TypeKind::Unknown) {
            diagnostics_.reportError(node.condition->location, "Condition must be boolean.");
        }
    }
    if (node.increment) node.increment->accept(*this);
    node.body->accept(*this);
    leaveScope();
}

void SemanticAnalyzer::visit(ExpressionStmt& node) {
    node.expression->accept(*this);
}

std::shared_ptr<Type> SemanticAnalyzer::resolveType(const TypeIdentifier& type_id, SourceLocation loc) {
    auto it = defined_types_.find(type_id.name);
    if (it == defined_types_.end()) {
        diagnostics_.reportError(loc, "Undefined type: " + std::string(type_id.name));
        return Type::getUnknown();
    }
    
    auto base_type = it->second;
    if (type_id.pointer_depth > 0 || type_id.is_array) {
        auto derived_type = std::make_shared<Type>(type_id.is_array ? TypeKind::Array : base_type->kind);
        derived_type->name = base_type->name;
        derived_type->pointer_depth = type_id.pointer_depth;
        if (type_id.is_array) {
            derived_type->element_type = base_type;
        }
        return derived_type;
    }
    return base_type;
}

// Stubs for now
void SemanticAnalyzer::visit(ArrayNode& node) { setNodeType(&node, Type::getUnknown()); }
void SemanticAnalyzer::visit(StructAccessNode& node) {
    node.object->accept(*this);
    auto obj_type = getNodeType(node.object.get());
    if (obj_type && obj_type->kind == TypeKind::Struct) {
        for (const auto& field : obj_type->struct_fields) {
            if (field.first == node.field.lexeme) {
                setNodeType(&node, field.second);
                return;
            }
        }
        diagnostics_.reportError(node.field.location, "Struct has no such field: " + std::string(node.field.lexeme));
    } else {
        diagnostics_.reportError(node.object->location, "Cannot access field of non-struct type.");
    }
    setNodeType(&node, Type::getUnknown());
}
void SemanticAnalyzer::visit(BreakStmt& node) {}
void SemanticAnalyzer::visit(ContinueStmt& node) {}
void SemanticAnalyzer::visit(StructNode& node) {
    auto it = defined_types_.find(node.name.lexeme);
    if (it != defined_types_.end()) {
        auto struct_type = it->second;
        for (const auto& field : node.fields) {
            auto field_type = resolveType(field.type, field.name.location);
            struct_type->struct_fields.push_back({std::string(field.name.lexeme), field_type});
        }
    }
}
void SemanticAnalyzer::visit(EnumNode& node) {}

} // namespace nova
