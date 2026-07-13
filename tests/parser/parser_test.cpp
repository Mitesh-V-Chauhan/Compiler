#include <gtest/gtest.h>
#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/ast/ast_dump.h"

using namespace nova;

std::string parseAndDump(std::string_view source) {
    Diagnostics diag(source);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens, diag);
    auto ast = parser.parse();
    
    if (diag.hasErrors()) {
        return "ERROR";
    }

    ASTDumper dumper;
    return dumper.dump(*ast);
}

TEST(ParserTest, BinaryExpression) {
    std::string dump = parseAndDump("1 + 2 * 3;");
    EXPECT_NE(dump, "ERROR");
    EXPECT_TRUE(dump.find("BinaryExpr [+]") != std::string::npos);
    EXPECT_TRUE(dump.find("BinaryExpr [*]") != std::string::npos);
}

TEST(ParserTest, LetStatement) {
    std::string dump = parseAndDump("let x: int = 10;");
    EXPECT_NE(dump, "ERROR");
    EXPECT_TRUE(dump.find("LetStmt [x]") != std::string::npos);
    EXPECT_TRUE(dump.find("Type: int") != std::string::npos);
    EXPECT_TRUE(dump.find("Literal [10]") != std::string::npos);
}

TEST(ParserTest, FunctionDeclaration) {
    std::string source = R"(
        fn factorial(n: int) -> int {
            if (n <= 1) { return 1; }
            return n * factorial(n - 1);
        }
    )";
    std::string dump = parseAndDump(source);
    EXPECT_NE(dump, "ERROR");
    EXPECT_TRUE(dump.find("Function [factorial]") != std::string::npos);
    EXPECT_TRUE(dump.find("Param: n : int") != std::string::npos);
    EXPECT_TRUE(dump.find("Return: int") != std::string::npos);
    EXPECT_TRUE(dump.find("IfStmt") != std::string::npos);
    EXPECT_TRUE(dump.find("ReturnStmt") != std::string::npos);
}

TEST(ParserTest, SyntaxError) {
    std::string dump = parseAndDump("let x: int = ;"); // missing initializer
    EXPECT_EQ(dump, "ERROR");
}
