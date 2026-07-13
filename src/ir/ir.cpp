#include "nova/ir/ir.h"
#include <iostream>

namespace nova {

std::string IRInstruction::toString() const {
    std::string res;
    if (dest.type != IRValue::Type::None) {
        res += dest.toString() + " = ";
    }
    
    switch (op) {
        case IROp::Add: res += "add "; break;
        case IROp::Sub: res += "sub "; break;
        case IROp::Mul: res += "mul "; break;
        case IROp::Div: res += "div "; break;
        case IROp::Mod: res += "mod "; break;
        case IROp::Eq: res += "eq "; break;
        case IROp::Neq: res += "neq "; break;
        case IROp::Lt: res += "lt "; break;
        case IROp::Lte: res += "lte "; break;
        case IROp::Gt: res += "gt "; break;
        case IROp::Gte: res += "gte "; break;
        case IROp::And: res += "and "; break;
        case IROp::Or: res += "or "; break;
        case IROp::Not: res += "not "; break;
        case IROp::Neg: res += "neg "; break;
        case IROp::Assign: res += "assign "; break;
        case IROp::Jump: res += "jmp " + dest.toString(); return res;
        case IROp::Branch: res += "br " + arg1.toString() + ", " + dest.toString() + ", " + arg2.toString(); return res;
        case IROp::Call: res += "call " + arg1.toString(); return res;
        case IROp::Return: res += "ret " + arg1.toString(); return res;
        case IROp::Param: res += "param " + arg1.toString(); return res;
        case IROp::Alloc: res += "alloc " + dest.toString(); return res;
    }
    
    if (arg1.type != IRValue::Type::None) res += arg1.toString();
    if (arg2.type != IRValue::Type::None) res += ", " + arg2.toString();
    
    return res;
}

void IRFunction::dump() const {
    std::cout << "define " << name << " {\n";
    for (const auto& block : blocks) {
        std::cout << block.label << ":\n";
        for (const auto& instr : block.instructions) {
            std::cout << "  " << instr.toString() << "\n";
        }
    }
    std::cout << "}\n\n";
}

void IRProgram::dump() const {
    for (const auto& func : functions) {
        func.dump();
    }
}

} // namespace nova
