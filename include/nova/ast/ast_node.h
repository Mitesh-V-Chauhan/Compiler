#pragma once
#include "nova/ast/visitor.h"
#include "nova/lexer/token.h"

namespace nova {

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
    
    SourceLocation location;
};

class Expression : public ASTNode {
};

class Statement : public ASTNode {
};

} // namespace nova
