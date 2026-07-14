# Nova

Nova is a statically typed, compiled systems programming language. It translates directly to bare-metal x86-64 assembly.

Built entirely from scratch in C++20, the Nova compiler features a handcrafted lexer, recursive descent parser, semantic analyzer, intermediate representation (IR) layer, optimizer, and an x86-64 code generator. It does not rely on parser generators (like Flex/Bison) or third-party compiler backends (like LLVM).

## Features

- **Zero Dependencies**: Hand-rolled Lexer, Parser, and AST.
- **Native Execution**: Compiles directly to x86-64 AT&T Assembly.
- **Systems Typing**: Supports `int`, `float`, `bool`, and pointers (`*`, `&`).
- **Composite Types**: Structs with strict memory layout calculation and padding.
- **Optimization Passes**: Constant folding, constant propagation, and dead code elimination.

## Pipeline Architecture

1. **Lexical Analysis**: Raw text to token streams.
2. **Parsing**: Recursive descent parser constructing the AST.
3. **Semantic Analysis**: Type checking, scope resolution, and memory sizing.
4. **IR Generation**: Lowering to a linear Three-Address Code (TAC) representation.
5. **Optimization**: Intra-block AST optimizations.
6. **Code Generation**: Translating IR to raw x86-64 instructions following the System V AMD64 ABI.

## Performance

Nova is a true native systems language. Because it skips interpreters, virtual machines, and garbage collectors, Nova programs execute at bare-metal hardware speeds. 

Since the compiler translates IR directly into raw CPU instructions and links executables via GCC/Clang, runtime execution performance theoretically mirrors C/C++ for equivalent logic. 

Furthermore, because the compiler is highly specialized and avoids massive monolithic backends, compilation speeds are exceptionally fast.

```bash
# Example end-to-end compilation speed
$ time ./build/src/nova source.nv
real	0m0.008s
user	0m0.004s
sys	    0m0.003s
```

## Build Instructions

Nova requires a C++20 compliant compiler and CMake.

```bash
# 1. Clone the repository
git clone https://github.com/Mitesh-V-Chauhan/Compiler.git nova
cd nova

# 2. Build via CMake
mkdir build && cd build
cmake ..
cmake --build . --config Release

# 3. Compile a Nova script
./src/nova ../examples/test.nv

# 4. Link and run the generated assembly natively
gcc -arch x86_64 test.nv.s -o a.out
./a.out
```

## Example Syntax

```rust
struct Point {
    x: int;
    y: int;
}

fn add(a: *int, b: *int) -> int {
    return *a + *b;
}

fn main() -> int {
    let p: Point;
    p.x = 20;
    p.y = 30;
    
    let ptr_x: *int = &p.x;
    let ptr_y: *int = &p.y;
    
    return add(ptr_x, ptr_y);
}
```

## License

MIT License
