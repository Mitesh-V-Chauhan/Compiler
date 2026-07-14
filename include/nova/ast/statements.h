#pragma once
#include "nova/ast/expressions.h"
#include <string_view>

namespace nova {

class BlockStmt : public Statement {
public:
    std::vector<std::unique_ptr<Statement>> statements;
    
    explicit BlockStmt(std::vector<std::unique_ptr<Statement>> stmts, SourceLocation loc)
        : statements(std::move(stmts)) { location = loc; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

// Represents a type parsing (e.g. `int`, `[int]`)
// Not an ASTNode directly visited, but part of nodes.
struct TypeIdentifier {
    std::string_view name;
    bool is_array = false;
    int pointer_depth = 0;
};

class LetStmt : public Statement {
public:
    Token identifier;
    std::unique_ptr<TypeIdentifier> type_id; // optional
    std::unique_ptr<Expression> initializer; // optional
    
    LetStmt(Token id, std::unique_ptr<TypeIdentifier> tid, std::unique_ptr<Expression> init)
        : identifier(id), type_id(std::move(tid)), initializer(std::move(init)) { location = id.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class IfStmt : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStmt> then_branch;
    std::unique_ptr<Statement> else_branch; // can be BlockStmt or IfStmt
    
    IfStmt(std::unique_ptr<Expression> cond, std::unique_ptr<BlockStmt> tb, std::unique_ptr<Statement> eb, SourceLocation loc)
        : condition(std::move(cond)), then_branch(std::move(tb)), else_branch(std::move(eb)) { location = loc; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class WhileStmt : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::unique_ptr<BlockStmt> body;
    
    WhileStmt(std::unique_ptr<Expression> cond, std::unique_ptr<BlockStmt> b, SourceLocation loc)
        : condition(std::move(cond)), body(std::move(b)) { location = loc; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ExpressionStmt : public Statement {
public:
    std::unique_ptr<Expression> expression;
    
    explicit ExpressionStmt(std::unique_ptr<Expression> expr)
        : expression(std::move(expr)) { location = expression->location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ReturnStmt : public Statement {
public:
    Token keyword;
    std::unique_ptr<Expression> value; // optional
    
    ReturnStmt(Token kw, std::unique_ptr<Expression> val)
        : keyword(kw), value(std::move(val)) { location = keyword.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class BreakStmt : public Statement {
public:
    Token keyword;
    explicit BreakStmt(Token kw) : keyword(kw) { location = kw.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ContinueStmt : public Statement {
public:
    Token keyword;
    explicit ContinueStmt(Token kw) : keyword(kw) { location = kw.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class FunctionNode : public ASTNode {
public:
    Token name;
    struct Param {
        Token name;
        TypeIdentifier type;
    };
    std::vector<Param> params;
    std::unique_ptr<TypeIdentifier> return_type; // null if void
    std::unique_ptr<BlockStmt> body;
    
    FunctionNode(Token n, std::vector<Param> p, std::unique_ptr<TypeIdentifier> rt, std::unique_ptr<BlockStmt> b)
        : name(n), params(std::move(p)), return_type(std::move(rt)), body(std::move(b)) { location = name.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> declarations;
    
    explicit ProgramNode(std::vector<std::unique_ptr<ASTNode>> decls)
        : declarations(std::move(decls)) {}
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class ForStmt : public Statement {
public:
    std::unique_ptr<Statement> initializer; // LetStmt or ExpressionStmt
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> increment;
    std::unique_ptr<BlockStmt> body;

    ForStmt(std::unique_ptr<Statement> init, std::unique_ptr<Expression> cond, std::unique_ptr<Expression> inc, std::unique_ptr<BlockStmt> b, SourceLocation loc)
        : initializer(std::move(init)), condition(std::move(cond)), increment(std::move(inc)), body(std::move(b)) { location = loc; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class StructNode : public ASTNode {
public:
    Token name;
    struct Field {
        Token name;
        TypeIdentifier type;
    };
    std::vector<Field> fields;

    StructNode(Token n, std::vector<Field> f)
        : name(n), fields(std::move(f)) { location = name.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class EnumNode : public ASTNode {
public:
    Token name;
    std::vector<Token> variants;

    EnumNode(Token n, std::vector<Token> v)
        : name(n), variants(std::move(v)) { location = name.location; }
    void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

} // namespace nova
