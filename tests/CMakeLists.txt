add_executable(nova_tests
    lexer/lexer_test.cpp
    parser/parser_test.cpp
)

target_link_libraries(nova_tests PRIVATE
    nova_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(nova_tests)
