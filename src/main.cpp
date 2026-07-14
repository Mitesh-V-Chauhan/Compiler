#include "nova/lexer/lexer.h"
#include "nova/parser/parser.h"
#include "nova/semantic/semantic_analyzer.h"
#include "nova/ir/ir_generator.h"
#include "nova/optimizer/optimizer.h"
#include "nova/backend/codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace nova;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: nova <file.nv>\n";
        return 1;
    }
    
    std::string filename = argv[1];
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filename << "\n";
        return 1;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    
    Diagnostics diag(source);
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    
    Parser parser(tokens, diag);
    auto ast = parser.parse();
    
    if (diag.hasErrors()) {
        return 1;
    }
    
    SemanticAnalyzer semantic(diag);
    semantic.analyze(*ast);
    
    if (diag.hasErrors()) {
        return 1;
    }
    
    IRGenerator ir_gen(&semantic);
    auto program = ir_gen.generate(*ast);
    
    Optimizer opt;
    opt.optimize(program);
    
    // Output assembly
    std::string asm_file = filename + ".s";
    std::ofstream out(asm_file);
    CodeGen codegen(out);
    codegen.generate(program);
    out.close();
    
    // Assemble and link via gcc (using -arch x86_64 for macOS Apple Silicon support)
    std::string out_exec = "a.out";
#ifdef __APPLE__
    std::string cmd = "gcc -arch x86_64 " + asm_file + " -o " + out_exec;
#else
    std::string cmd = "gcc " + asm_file + " -o " + out_exec;
#endif
    int ret = system(cmd.c_str());
    if (ret == 0) {
        std::cout << "Successfully compiled to " << out_exec << "\n";
    } else {
        std::cerr << "Linking failed.\n";
    }
    
    return ret;
}
