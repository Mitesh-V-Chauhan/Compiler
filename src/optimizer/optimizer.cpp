#include "nova/optimizer/optimizer.h"
#include <unordered_map>
#include <unordered_set>

namespace nova {

void Optimizer::optimize(IRProgram& program) {
    for (auto& func : program.functions) {
        optimizeFunction(func);
    }
}

void Optimizer::optimizeFunction(IRFunction& func) {
    bool changed = true;
    while (changed) {
        changed = false;
        changed |= constantFolding(func);
        changed |= constantPropagation(func);
        changed |= deadCodeElimination(func);
        changed |= controlFlowSimplification(func);
    }
}

bool Optimizer::constantFolding(IRFunction& func) {
    bool changed = false;
    for (auto& block : func.blocks) {
        for (auto& instr : block.instructions) {
            if (instr.arg1.type == IRValue::Type::Constant && instr.arg2.type == IRValue::Type::Constant) {
                int a = std::stoi(instr.arg1.name);
                int b = std::stoi(instr.arg2.name);
                int res = 0;
                bool folded = true;
                
                switch (instr.op) {
                    case IROp::Add: res = a + b; break;
                    case IROp::Sub: res = a - b; break;
                    case IROp::Mul: res = a * b; break;
                    case IROp::Div: if (b != 0) res = a / b; else folded = false; break;
                    case IROp::Mod: if (b != 0) res = a % b; else folded = false; break;
                    case IROp::Eq: res = (a == b); break;
                    case IROp::Neq: res = (a != b); break;
                    case IROp::Lt: res = (a < b); break;
                    case IROp::Lte: res = (a <= b); break;
                    case IROp::Gt: res = (a > b); break;
                    case IROp::Gte: res = (a >= b); break;
                    case IROp::And: res = (a && b); break;
                    case IROp::Or: res = (a || b); break;
                    default: folded = false; break;
                }
                
                if (folded) {
                    instr.op = IROp::Assign;
                    instr.arg1 = {IRValue::Type::Constant, std::to_string(res)};
                    instr.arg2 = {IRValue::Type::None, ""};
                    changed = true;
                }
            } else if (instr.op == IROp::Not && instr.arg1.type == IRValue::Type::Constant) {
                int a = std::stoi(instr.arg1.name);
                instr.op = IROp::Assign;
                instr.arg1 = {IRValue::Type::Constant, std::to_string(!a)};
                changed = true;
            } else if (instr.op == IROp::Neg && instr.arg1.type == IRValue::Type::Constant) {
                int a = std::stoi(instr.arg1.name);
                instr.op = IROp::Assign;
                instr.arg1 = {IRValue::Type::Constant, std::to_string(-a)};
                changed = true;
            }
        }
    }
    return changed;
}

bool Optimizer::constantPropagation(IRFunction& func) {
    bool changed = false;
    std::unordered_map<std::string, std::string> constants;
    
    // Very simple intra-block propagation
    for (auto& block : func.blocks) {
        constants.clear();
        for (auto& instr : block.instructions) {
            if ((instr.arg1.type == IRValue::Type::Temp || instr.arg1.type == IRValue::Type::Variable) && constants.count(instr.arg1.name)) {
                instr.arg1 = {IRValue::Type::Constant, constants[instr.arg1.name]};
                changed = true;
            }
            if ((instr.arg2.type == IRValue::Type::Temp || instr.arg2.type == IRValue::Type::Variable) && constants.count(instr.arg2.name)) {
                instr.arg2 = {IRValue::Type::Constant, constants[instr.arg2.name]};
                changed = true;
            }
            
            if (instr.op == IROp::Assign && instr.arg1.type == IRValue::Type::Constant && 
               (instr.dest.type == IRValue::Type::Temp || instr.dest.type == IRValue::Type::Variable)) {
                constants[instr.dest.name] = instr.arg1.name;
            } else if (instr.dest.type == IRValue::Type::Temp || instr.dest.type == IRValue::Type::Variable) {
                constants.erase(instr.dest.name);
            }
        }
    }
    return changed;
}

bool Optimizer::deadCodeElimination(IRFunction& func) {
    bool changed = false;
    std::unordered_set<std::string> used_temps;
    
    for (const auto& block : func.blocks) {
        for (const auto& instr : block.instructions) {
            if (instr.arg1.type == IRValue::Type::Temp) used_temps.insert(instr.arg1.name);
            if (instr.arg2.type == IRValue::Type::Temp) used_temps.insert(instr.arg2.name);
        }
    }
    
    for (auto& block : func.blocks) {
        auto it = block.instructions.begin();
        while (it != block.instructions.end()) {
            if (it->dest.type == IRValue::Type::Temp && used_temps.find(it->dest.name) == used_temps.end() && it->op != IROp::Call) {
                it = block.instructions.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    }
    return changed;
}

bool Optimizer::controlFlowSimplification(IRFunction& func) {
    // A simple pass: if a branch has a constant condition, turn it into a jump
    bool changed = false;
    for (auto& block : func.blocks) {
        for (auto& instr : block.instructions) {
            if (instr.op == IROp::Branch && instr.arg1.type == IRValue::Type::Constant) {
                int cond = std::stoi(instr.arg1.name);
                std::string target = cond ? instr.dest.name : instr.arg2.name;
                
                instr.op = IROp::Jump;
                instr.dest = {IRValue::Type::Label, target};
                instr.arg1 = {IRValue::Type::None, ""};
                instr.arg2 = {IRValue::Type::None, ""};
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace nova
