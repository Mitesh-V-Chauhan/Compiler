#include "nova/backend/codegen.h"
#include <stdexcept>
#include <unordered_set>

namespace nova {

CodeGen::CodeGen(std::ostream& out) : out_(out) {}

void CodeGen::generate(const IRProgram& program) {
    out_ << ".text\n";
    for (const auto& func : program.functions) {
        generateFunction(func);
    }
    
    // Standard Prelude Runtime Helpers
    std::unordered_set<std::string> user_funcs;
    for (const auto& func : program.functions) {
        user_funcs.insert(func.name);
    }

    out_ << ".text\n";
    if (user_funcs.find("print") == user_funcs.end()) {
        out_ << ".globl _print\n_print:\n"
             << "    pushq %rbp\n    movq %rsp, %rbp\n"
             << "    movq %rdi, %rsi\n"
             << "    leaq .L.prelude_str(%rip), %rdi\n"
             << "    xor %eax, %eax\n"
             << "    call _printf\n"
             << "    movq %rbp, %rsp\n    popq %rbp\n    ret\n";
    }
    if (user_funcs.find("println") == user_funcs.end()) {
        out_ << ".globl _println\n_println:\n"
             << "    pushq %rbp\n    movq %rsp, %rbp\n"
             << "    call _puts\n"
             << "    movq %rbp, %rsp\n    popq %rbp\n    ret\n";
    }
    if (user_funcs.find("print_int") == user_funcs.end()) {
        out_ << ".globl _print_int\n_print_int:\n"
             << "    pushq %rbp\n    movq %rsp, %rbp\n"
             << "    movq %rdi, %rsi\n"
             << "    leaq .L.prelude_int(%rip), %rdi\n"
             << "    xor %eax, %eax\n"
             << "    call _printf\n"
             << "    movq %rbp, %rsp\n    popq %rbp\n    ret\n";
    }
    if (user_funcs.find("alloc") == user_funcs.end()) {
        out_ << ".globl _alloc\n_alloc:\n"
             << "    pushq %rbp\n    movq %rsp, %rbp\n"
             << "    call _malloc\n"
             << "    movq %rbp, %rsp\n    popq %rbp\n    ret\n";
    }
    if (user_funcs.find("dealloc") == user_funcs.end()) {
        out_ << ".globl _dealloc\n_dealloc:\n"
             << "    pushq %rbp\n    movq %rsp, %rbp\n"
             << "    call _free\n"
             << "    movq %rbp, %rsp\n    popq %rbp\n    ret\n";
    }

#ifdef __APPLE__
    out_ << ".section __TEXT,__cstring,cstring_literals\n";
#else
    out_ << ".section .rodata\n";
#endif
    out_ << ".L.prelude_str:\n    .asciz \"%s\"\n";
    out_ << ".L.prelude_int:\n    .asciz \"%lld\\n\"\n";

    if (!program.strings.empty()) {
        for (const auto& str : program.strings) {
            out_ << str.first << ":\n";
            out_ << "    .asciz \"" << escapeString(str.second) << "\"\n";
        }
    }

    if (!program.globals.empty()) {
        bool has_data = false;
        for (const auto& g : program.globals) {
            if (g.has_init) {
                if (!has_data) {
                    out_ << ".data\n";
                    has_data = true;
                }
                out_ << ".globl _" << g.name << "\n";
                out_ << ".p2align 3\n";
                out_ << "_" << g.name << ":\n";
                out_ << "    .quad " << g.init_val << "\n";
            } else {
                out_ << ".comm _" << g.name << ", " << g.size << ", 3\n";
            }
        }
    }
}

std::string CodeGen::escapeString(const std::string& str) {
    std::string res;
    for (char c : str) {
        switch (c) {
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            case '\"': res += "\\\""; break;
            case '\\': res += "\\\\"; break;
            default: res += c; break;
        }
    }
    return res;
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
    } else if (val.type == IRValue::Type::Label) {
        out_ << "    leaq " << val.name << "(%rip), " << reg << "\n";
    } else if (val.type == IRValue::Type::Global) {
        out_ << "    movq _" << val.name << "(%rip), " << reg << "\n";
    }
    return reg;
}

std::string CodeGen::loadAddress(const IRValue& val, const std::string& reg) {
    if (val.type == IRValue::Type::Variable || val.type == IRValue::Type::Temp) {
        int offset = getStackOffset(val.name);
        out_ << "    leaq " << offset << "(%rbp), " << reg << "\n";
    } else if (val.type == IRValue::Type::Global) {
        out_ << "    leaq _" << val.name << "(%rip), " << reg << "\n";
    }
    return reg;
}

void CodeGen::storeValue(const IRValue& val, const std::string& reg) {
    if (val.type == IRValue::Type::Variable || val.type == IRValue::Type::Temp) {
        int offset = getStackOffset(val.name);
        out_ << "    movq " << reg << ", " << offset << "(%rbp)\n";
    } else if (val.type == IRValue::Type::Global) {
        out_ << "    movq " << reg << ", _" << val.name << "(%rip)\n";
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
            out_ << "    xor %eax, %eax\n"; // System V ABI: zero %al for variadic functions
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
