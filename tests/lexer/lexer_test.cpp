#include <gtest/gtest.h>
#include "nova/lexer/lexer.h"

using namespace nova;

TEST(LexerTest, EmptySource) {
    Lexer lexer("");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::Eof);
}

TEST(LexerTest, Keywords) {
    Lexer lexer("fn let if else while return struct enum");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].type, TokenType::Fn);
    EXPECT_EQ(tokens[1].type, TokenType::Let);
    EXPECT_EQ(tokens[2].type, TokenType::If);
    EXPECT_EQ(tokens[3].type, TokenType::Else);
    EXPECT_EQ(tokens[4].type, TokenType::While);
    EXPECT_EQ(tokens[5].type, TokenType::Return);
    EXPECT_EQ(tokens[6].type, TokenType::Struct);
    EXPECT_EQ(tokens[7].type, TokenType::Enum);
    EXPECT_EQ(tokens[8].type, TokenType::Eof);
}

TEST(LexerTest, Identifiers) {
    Lexer lexer("myVar _var123 struct_name");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[0].lexeme, "myVar");
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].lexeme, "_var123");
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].lexeme, "struct_name");
}

TEST(LexerTest, Numbers) {
    Lexer lexer("123 3.14 0 0.5");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[0].lexeme, "123");
    EXPECT_EQ(tokens[1].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[1].lexeme, "3.14");
    EXPECT_EQ(tokens[2].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[2].lexeme, "0");
    EXPECT_EQ(tokens[3].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[3].lexeme, "0.5");
}

TEST(LexerTest, Strings) {
    Lexer lexer(R"("hello" "world")");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].lexeme, R"("hello")");
    EXPECT_EQ(tokens[1].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[1].lexeme, R"("world")");
}

TEST(LexerTest, Characters) {
    Lexer lexer("'a' '\\n'");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::CharLiteral);
    EXPECT_EQ(tokens[0].lexeme, "'a'");
    EXPECT_EQ(tokens[1].type, TokenType::CharLiteral);
    EXPECT_EQ(tokens[1].lexeme, "'\\n'");
}

TEST(LexerTest, Operators) {
    Lexer lexer("+ - * / % == != > < >= <= = && || ! ->");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 17);
    EXPECT_EQ(tokens[0].type, TokenType::Plus);
    EXPECT_EQ(tokens[1].type, TokenType::Minus);
    EXPECT_EQ(tokens[2].type, TokenType::Star);
    EXPECT_EQ(tokens[3].type, TokenType::Slash);
    EXPECT_EQ(tokens[4].type, TokenType::Percent);
    EXPECT_EQ(tokens[5].type, TokenType::EqEq);
    EXPECT_EQ(tokens[6].type, TokenType::BangEq);
    EXPECT_EQ(tokens[7].type, TokenType::Greater);
    EXPECT_EQ(tokens[8].type, TokenType::Less);
    EXPECT_EQ(tokens[9].type, TokenType::GreaterEq);
    EXPECT_EQ(tokens[10].type, TokenType::LessEq);
    EXPECT_EQ(tokens[11].type, TokenType::Eq);
    EXPECT_EQ(tokens[12].type, TokenType::AndAnd);
    EXPECT_EQ(tokens[13].type, TokenType::OrOr);
    EXPECT_EQ(tokens[14].type, TokenType::Bang);
    EXPECT_EQ(tokens[15].type, TokenType::Arrow);
}

TEST(LexerTest, Comments) {
    Lexer lexer("123 // this is a comment\n456");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[0].lexeme, "123");
    EXPECT_EQ(tokens[1].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[1].lexeme, "456");
}

TEST(LexerTest, SourceLocationTracking) {
    Lexer lexer("let x = 10;\n  return x;");
    auto tokens = lexer.tokenize();
    
    EXPECT_EQ(tokens[0].type, TokenType::Let);
    EXPECT_EQ(tokens[0].location.line, 1);
    EXPECT_EQ(tokens[0].location.column, 1);
    
    EXPECT_EQ(tokens[1].type, TokenType::Identifier); // x
    EXPECT_EQ(tokens[1].location.line, 1);
    EXPECT_EQ(tokens[1].location.column, 5);
    
    EXPECT_EQ(tokens[5].type, TokenType::Return);
    EXPECT_EQ(tokens[5].location.line, 2);
    EXPECT_EQ(tokens[5].location.column, 3);
}

TEST(LexerTest, ErrorTokens) {
    Lexer lexer("@ #");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::Error);
    EXPECT_EQ(tokens[1].type, TokenType::Error);
}
