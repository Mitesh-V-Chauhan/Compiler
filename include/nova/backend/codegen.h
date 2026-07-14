#pragma once
#include "nova/ir/ir.h"
#include <string>
#include <ostream>
#include <unordered_map>
#include <vector>

namespace nova {

class CodeGen {
public:
    explicit CodeGen(std::ostream& out);
    void generate(const IRProgram& program);

private:
    std::ostream& out_;
    
    void generateFunction(const IRFunction& func);
    void generateBlock(const BasicBlock& block);
    void generateInstruction(const IRInstruction& instr);
    
    std::unordered_map<std::string, int> stack_offsets_;
    int current_stack_offset_ = 0;
    
    int getStackOffset(const std::string& name);
    std::string loadValue(const IRValue& val, const std::string& reg);
    std::string loadAddress(const IRValue& val, const std::string& reg);
    void storeValue(const IRValue& val, const std::string& reg);
    
    const std::vector<std::string> arg_regs_ = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
    int param_count_ = 0;
    int arg_count_ = 0;
};

} // namespace nova
