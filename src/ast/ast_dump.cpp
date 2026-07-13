#include "nova/ast/ast_dump.h"
#include "nova/ast/expressions.h"
#include "nova/ast/statements.h"
#include <sstream>

namespace nova {

std::string ASTDumper::dump(ASTNode& node) {
    output_.clear();
    indent_level_ = 0;
    node.accept(*this);
    return output_;
}

void ASTDumper::indent() {
    output_ += std::string(indent_level_ * 2, ' ');
}

void ASTDumper::visit(ProgramNode& node) {
    output_ += "Program\n";
    indent_level_++;
    for (auto& decl : node.declarations) {
        indent();
        decl->accept(*this);
    }
    indent_level_--;
}

void ASTDumper::visit(BinaryExpression& node) {
    output_ += "BinaryExpr [" + std::string(node.op.lexeme) + "]\n";
    indent_level_++;
    indent(); node.left->accept(*this);
    indent(); node.right->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(UnaryExpression& node) {
    output_ += "UnaryExpr [" + std::string(node.op.lexeme) + "]\n";
    indent_level_++;
    indent(); node.operand->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(LiteralNode& node) {
    output_ += "Literal [" + std::string(node.token.lexeme) + "]\n";
}

void ASTDumper::visit(IdentifierNode& node) {
    output_ += "Identifier [" + std::string(node.token.lexeme) + "]\n";
}

void ASTDumper::visit(CallNode& node) {
    output_ += "Call\n";
    indent_level_++;
    indent(); output_ += "Callee:\n";
    indent_level_++; indent(); node.callee->accept(*this); indent_level_--;
    indent(); output_ += "Args:\n";
    indent_level_++;
    for (auto& arg : node.arguments) {
        indent(); arg->accept(*this);
    }
    indent_level_--;
    indent_level_--;
}

void ASTDumper::visit(ArrayNode& node) {
    output_ += "Array\n";
    indent_level_++;
    for (auto& elem : node.elements) {
        indent(); elem->accept(*this);
    }
    indent_level_--;
}

void ASTDumper::visit(StructAccessNode& node) {
    output_ += "StructAccess [" + std::string(node.field.lexeme) + "]\n";
    indent_level_++;
    indent(); node.object->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(LetStmt& node) {
    output_ += "LetStmt [" + std::string(node.identifier.lexeme) + "]\n";
    indent_level_++;
    if (node.type_id) {
        indent(); output_ += "Type: " + std::string(node.type_id->name) + (node.type_id->is_array ? "[]\n" : "\n");
    }
    if (node.initializer) {
        indent(); node.initializer->accept(*this);
    }
    indent_level_--;
}

void ASTDumper::visit(IfStmt& node) {
    output_ += "IfStmt\n";
    indent_level_++;
    indent(); output_ += "Condition:\n";
    indent_level_++; indent(); node.condition->accept(*this); indent_level_--;
    indent(); output_ += "Then:\n";
    indent_level_++; indent(); node.then_branch->accept(*this); indent_level_--;
    if (node.else_branch) {
        indent(); output_ += "Else:\n";
        indent_level_++; indent(); node.else_branch->accept(*this); indent_level_--;
    }
    indent_level_--;
}

void ASTDumper::visit(WhileStmt& node) {
    output_ += "WhileStmt\n";
    indent_level_++;
    indent(); node.condition->accept(*this);
    indent(); node.body->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(ForStmt& node) {
    output_ += "ForStmt\n";
    indent_level_++;
    if (node.initializer) { indent(); node.initializer->accept(*this); }
    if (node.condition) { indent(); node.condition->accept(*this); }
    if (node.increment) { indent(); node.increment->accept(*this); }
    indent(); node.body->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(ReturnStmt& node) {
    output_ += "ReturnStmt\n";
    if (node.value) {
        indent_level_++;
        indent(); node.value->accept(*this);
        indent_level_--;
    }
}

void ASTDumper::visit(BlockStmt& node) {
    output_ += "BlockStmt\n";
    indent_level_++;
    for (auto& stmt : node.statements) {
        indent(); stmt->accept(*this);
    }
    indent_level_--;
}

void ASTDumper::visit(ExpressionStmt& node) {
    output_ += "ExpressionStmt\n";
    indent_level_++;
    indent(); node.expression->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(BreakStmt& node) {
    output_ += "BreakStmt\n";
}

void ASTDumper::visit(ContinueStmt& node) {
    output_ += "ContinueStmt\n";
}

void ASTDumper::visit(FunctionNode& node) {
    output_ += "Function [" + std::string(node.name.lexeme) + "]\n";
    indent_level_++;
    for (auto& p : node.params) {
        indent(); output_ += "Param: " + std::string(p.name.lexeme) + " : " + std::string(p.type.name) + "\n";
    }
    if (node.return_type) {
        indent(); output_ += "Return: " + std::string(node.return_type->name) + "\n";
    }
    indent(); node.body->accept(*this);
    indent_level_--;
}

void ASTDumper::visit(StructNode& node) {
    output_ += "Struct [" + std::string(node.name.lexeme) + "]\n";
    indent_level_++;
    for (auto& f : node.fields) {
        indent(); output_ += "Field: " + std::string(f.name.lexeme) + " : " + std::string(f.type.name) + "\n";
    }
    indent_level_--;
}

void ASTDumper::visit(EnumNode& node) {
    output_ += "Enum [" + std::string(node.name.lexeme) + "]\n";
    indent_level_++;
    for (auto& v : node.variants) {
        indent(); output_ += "Variant: " + std::string(v.lexeme) + "\n";
    }
    indent_level_--;
}

} // namespace nova
