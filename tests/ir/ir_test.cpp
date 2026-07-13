#include <gtest/gtest.h>
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/ir/ir_generator.h"

using namespace nova;

IRProgram generateIR(std::string_view source) {
    Diagnostics diag(source);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens, diag);
    auto ast = parser.parse();
    
    SemanticAnalyzer analyzer(diag);
    analyzer.analyze(*ast);
    
    IRGenerator generator;
    return generator.generate(*ast);
}

TEST(IRTest, BinaryExpression) {
    std::string source = R"(
        fn main() {
            let x = 10 + 20 * 30;
        }
    )";
    auto program = generateIR(source);
    ASSERT_EQ(program.functions.size(), 1);
    
    const auto& func = program.functions[0];
    ASSERT_EQ(func.blocks.size(), 1);
    
    const auto& block = func.blocks[0];
    bool has_add = false;
    bool has_mul = false;
    for (const auto& instr : block.instructions) {
        if (instr.op == IROp::Add) has_add = true;
        if (instr.op == IROp::Mul) has_mul = true;
    }
    EXPECT_TRUE(has_add);
    EXPECT_TRUE(has_mul);
}

TEST(IRTest, IfStatement) {
    std::string source = R"(
        fn main() {
            if (1 < 2) {
                let x = 1;
            } else {
                let x = 2;
            }
        }
    )";
    auto program = generateIR(source);
    ASSERT_EQ(program.functions.size(), 1);
    const auto& func = program.functions[0];
    // Entry, Then, Else, Endif
    EXPECT_GE(func.blocks.size(), 4);
}
