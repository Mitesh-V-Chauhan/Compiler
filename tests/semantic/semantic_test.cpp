#include <gtest/gtest.h>
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"

using namespace nova;

bool analyzeCode(std::string_view source) {
    Diagnostics diag(source);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens, diag);
    auto ast = parser.parse();
    
    if (diag.hasErrors()) {
        return false;
    }

    SemanticAnalyzer analyzer(diag);
    analyzer.analyze(*ast);
    return !diag.hasErrors();
}

TEST(SemanticTest, ValidCode) {
    std::string source = R"(
        fn add(a: int, b: int) -> int {
            return a + b;
        }

        fn main() {
            let x: int = 10;
            let y = 20;
            let z = add(x, y);
            
            if (z > 15) {
                let msg = "Hello";
            }
        }
    )";
    EXPECT_TRUE(analyzeCode(source));
}

TEST(SemanticTest, UndefinedVariable) {
    std::string source = "fn main() { let x = y; }";
    EXPECT_FALSE(analyzeCode(source));
}

TEST(SemanticTest, TypeMismatchInAssignment) {
    std::string source = "fn main() { let x: int = 10; x = \"hello\"; }";
    EXPECT_FALSE(analyzeCode(source));
}

TEST(SemanticTest, TypeMismatchInBinaryOp) {
    std::string source = "fn main() { let x = 10 + \"hello\"; }";
    EXPECT_FALSE(analyzeCode(source));
}

TEST(SemanticTest, ReturnTypeMismatch) {
    std::string source = "fn foo() -> int { return \"string\"; }";
    EXPECT_FALSE(analyzeCode(source));
}

TEST(SemanticTest, DuplicateDeclaration) {
    std::string source = "fn main() { let x = 10; let x = 20; }";
    EXPECT_FALSE(analyzeCode(source));
}
