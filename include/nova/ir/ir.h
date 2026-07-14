#pragma once
#include <string>
#include <vector>
#include <iostream>

namespace nova {

enum class IROp {
    Add, Sub, Mul, Div, Mod,
    Eq, Neq, Lt, Lte, Gt, Gte,
    And, Or, Not, Neg,
    Assign,
    Jump, Branch,
    Call, Return, Param, GetParam,
    Alloc,
    Load, Store, GetElementPtr, AddressOf
};

struct IRValue {
    enum class Type { Temp, Variable, Constant, Label, None } type = Type::None;
    std::string name;
    
    std::string toString() const { return name; }
};

struct IRInstruction {
    IROp op;
    IRValue dest;
    IRValue arg1;
    IRValue arg2;
    
    std::string toString() const;
};

class BasicBlock {
public:
    std::string label;
    std::vector<IRInstruction> instructions;
};

class IRFunction {
public:
    std::string name;
    std::vector<BasicBlock> blocks;
    
    void dump() const;
};

class IRProgram {
public:
    std::vector<IRFunction> functions;
    std::vector<std::pair<std::string, std::string>> strings;
    
    void dump() const;
};

} // namespace nova
