#include "nova/backend/codegen.h"
#include <stdexcept>

namespace nova {

CodeGen::CodeGen(std::ostream& out) : out_(out) {}

void CodeGen::generate(const IRProgram& program) {
    out_ << ".text\n";
    for (const auto& func : program.functions) {
        generateFunction(func);
    }
}

int CodeGen::getStackOffset(const std::string& name) {
    auto it = stack_offsets_.find(name);
    if (it != stack_offsets_.end()) {
        return it->second;
    }
    current_stack_offset_ -= 8; // Default 8 bytes for undeclared temps
    stack_offsets_[name] = current_stack_offset_;
    return current_stack_offset_;
}

std::string CodeGen::loadValue(const IRValue& val, const std::string& reg) {
    if (val.type == IRValue::Type::Constant) {
        out_ << "    movq $" << val.name << ", " << reg << "\n";
    } else if (val.type == IRValue::Type::Variable || val.type == IRValue::Type::Temp) {
        int offset = getStackOffset(val.name);
        out_ << "    movq " << offset << "(%rbp), " << reg << "\n";
    }
    return reg;
}

std::string CodeGen::loadAddress(const IRValue& val, const std::string& reg) {
    if (val.type == IRValue::Type::Variable || val.type == IRValue::Type::Temp) {
        int offset = getStackOffset(val.name);
        out_ << "    leaq " << offset << "(%rbp), " << reg << "\n";
    }
    return reg;
}

void CodeGen::storeValue(const IRValue& val, const std::string& reg) {
    if (val.type == IRValue::Type::Variable || val.type == IRValue::Type::Temp) {
        int offset = getStackOffset(val.name);
        out_ << "    movq " << reg << ", " << offset << "(%rbp)\n";
    }
}

void CodeGen::generateFunction(const IRFunction& func) {
    // Check if main, macOS needs _main, Linux needs main. Let's output _main by default for macOS.
    out_ << ".globl _" << func.name << "\n";
    out_ << "_" << func.name << ":\n";
    
    // Prologue
    out_ << "    pushq %rbp\n";
    out_ << "    movq %rsp, %rbp\n";
    
    stack_offsets_.clear();
    current_stack_offset_ = 0;
    param_count_ = 0;
    
    out_ << "    subq $256, %rsp\n";

    for (const auto& block : func.blocks) {
        generateBlock(block);
    }
}

void CodeGen::generateBlock(const BasicBlock& block) {
    out_ << block.label << ":\n";
    for (const auto& instr : block.instructions) {
        generateInstruction(instr);
    }
}

void CodeGen::generateInstruction(const IRInstruction& instr) {
    switch (instr.op) {
        case IROp::Add:
        case IROp::Sub:
        case IROp::Mul:
            loadValue(instr.arg1, "%rax");
            if (instr.arg2.type == IRValue::Type::Constant) {
                if (instr.op == IROp::Add) out_ << "    addq $" << instr.arg2.name << ", %rax\n";
                else if (instr.op == IROp::Sub) out_ << "    subq $" << instr.arg2.name << ", %rax\n";
                else out_ << "    imulq $" << instr.arg2.name << ", %rax, %rax\n";
            } else {
                loadValue(instr.arg2, "%rcx");
                if (instr.op == IROp::Add) out_ << "    addq %rcx, %rax\n";
                else if (instr.op == IROp::Sub) out_ << "    subq %rcx, %rax\n";
                else out_ << "    imulq %rcx, %rax\n";
            }
            storeValue(instr.dest, "%rax");
            break;
            
        case IROp::Div:
        case IROp::Mod:
            loadValue(instr.arg1, "%rax");
            out_ << "    cqto\n";
            loadValue(instr.arg2, "%rcx");
            out_ << "    idivq %rcx\n";
            if (instr.op == IROp::Div) storeValue(instr.dest, "%rax");
            else storeValue(instr.dest, "%rdx");
            break;
            
        case IROp::Assign:
            loadValue(instr.arg1, "%rax");
            storeValue(instr.dest, "%rax");
            break;
            
        case IROp::Eq:
        case IROp::Neq:
        case IROp::Lt:
        case IROp::Lte:
        case IROp::Gt:
        case IROp::Gte: {
            loadValue(instr.arg1, "%rax");
            if (instr.arg2.type == IRValue::Type::Constant) {
                out_ << "    cmpq $" << instr.arg2.name << ", %rax\n";
            } else {
                loadValue(instr.arg2, "%rcx");
                out_ << "    cmpq %rcx, %rax\n";
            }
            std::string set_instr;
            switch (instr.op) {
                case IROp::Eq: set_instr = "sete"; break;
                case IROp::Neq: set_instr = "setne"; break;
                case IROp::Lt: set_instr = "setl"; break;
                case IROp::Lte: set_instr = "setle"; break;
                case IROp::Gt: set_instr = "setg"; break;
                case IROp::Gte: set_instr = "setge"; break;
                default: break;
            }
            out_ << "    " << set_instr << " %al\n";
            out_ << "    movzbq %al, %rax\n";
            storeValue(instr.dest, "%rax");
            break;
        }
            
        case IROp::Jump:
            out_ << "    jmp " << instr.dest.name << "\n";
            break;
            
        case IROp::Branch:
            loadValue(instr.arg1, "%rax");
            out_ << "    testq %rax, %rax\n";
            out_ << "    jnz " << instr.dest.name << "\n";
            out_ << "    jmp " << instr.arg2.name << "\n";
            break;
            
        case IROp::Param: {
            loadValue(instr.arg1, "%rax");
            if (arg_count_ < arg_regs_.size()) {
                out_ << "    movq %rax, " << arg_regs_[arg_count_] << "\n";
                arg_count_++;
            } else {
                out_ << "    pushq %rax\n";
                arg_count_++;
            }
            break;
        }

        case IROp::GetParam: {
            int index = std::stoi(instr.arg1.name);
            if (index < arg_regs_.size()) {
                storeValue(instr.dest, arg_regs_[index]);
            } else {
                // Not supported yet
            }
            break;
        }

        case IROp::Call:
            out_ << "    call _" << instr.arg1.name << "\n";
            storeValue(instr.dest, "%rax");
            arg_count_ = 0;
            break;
            
        case IROp::Return:
            if (instr.arg1.type != IRValue::Type::None) {
                loadValue(instr.arg1, "%rax");
            }
            // Epilogue
            out_ << "    movq %rbp, %rsp\n";
            out_ << "    popq %rbp\n";
            out_ << "    ret\n";
            break;
            
        case IROp::Alloc: {
            int size = 8;
            if (instr.arg1.type == IRValue::Type::Constant) {
                size = std::stoi(instr.arg1.name);
            }
            if (size % 8 != 0) size += 8 - (size % 8);
            current_stack_offset_ -= size;
            stack_offsets_[instr.dest.name] = current_stack_offset_;
            break;
        }
            
        case IROp::GetElementPtr: {
            loadAddress(instr.arg1, "%rax");
            if (instr.arg2.type == IRValue::Type::Constant) {
                out_ << "    addq $" << instr.arg2.name << ", %rax\n";
            } else {
                loadValue(instr.arg2, "%rcx");
                out_ << "    addq %rcx, %rax\n";
            }
            storeValue(instr.dest, "%rax");
            break;
        }
            
        case IROp::Load: {
            loadValue(instr.arg1, "%rax");
            out_ << "    movq (%rax), %rcx\n";
            storeValue(instr.dest, "%rcx");
            break;
        }
            
        case IROp::Store: {
            loadValue(instr.arg2, "%rcx");
            loadValue(instr.arg1, "%rax");
            out_ << "    movq %rcx, (%rax)\n";
            break;
        }
            
        case IROp::AddressOf: {
            loadAddress(instr.arg1, "%rax");
            storeValue(instr.dest, "%rax");
            break;
        }
            
        default:
            break;
    }
}

} // namespace nova
