add_library(nova_core
    lexer/lexer.cpp
    lexer/token.cpp
    ast/ast_dump.cpp
    diagnostics/diagnostics.cpp
    parser/parser.cpp
    semantic/semantic_analyzer.cpp
    ir/ir.cpp
    ir/ir_generator.cpp
    optimizer/optimizer.cpp
    backend/codegen.cpp
    backend/memory_layout.cpp
)
target_include_directories(nova_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(nova main.cpp)
target_link_libraries(nova PRIVATE nova_core)
