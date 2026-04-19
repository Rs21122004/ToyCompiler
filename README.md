# ToyCompiler — Simple Language → PTX Backend

A compiler written in **C++17** that translates a statically-typed imperative language
into **NVIDIA PTX virtual assembly** — the same intermediate representation used by
CUDA compilers before `ptxas` lowers it to device-specific machine code.

Built as a hands-on exploration of the compiler pipeline relevant to the NVIDIA PTX
Compiler (PTXAS) team: front-end (lexer → parser → AST), semantic analysis
(scope/symbol table), optimisation passes (constant folding), and a register-based
PTX code generator.

---

## Pipeline

```
source.toy
    │
    ▼
┌─────────┐     ┌─────────┐     ┌──────────┐     ┌──────────────┐     ┌─────────┐
│  Lexer  │────▶│ Parser  │────▶│  Sema    │────▶│  Optimizer   │────▶│ Codegen │
│(DFA-    │     │(Recurs. │     │(Scope    │     │(Const. fold, │     │(PTX .s32│
│ based)  │     │ descent)│     │ analysis,│     │ identity     │     │ register│
│         │     │         │     │ use-     │     │ elim.,       │     │ alloc.) │
│Token[]  │     │ AST     │     │ before-  │     │ zero annihil)│     │         │
└─────────┘     └─────────┘     │ declare) │     └──────────────┘     └─────────┘
                                └──────────┘                               │
                                                                           ▼
                                                                      output.ptx
```

---

## Language Features

```c
// Variable declaration (32-bit signed integer — maps to PTX .s32)
int x = 10;
int y;          // default-initialised to 0

// Arithmetic: + - * / %
int z = x * 2 + y - 1;

// Comparison operators: == != < <= > >=
// Logical operators:    && || !

// if / else
if (x > 5) {
    y = x - 5;
} else {
    y = 0;
}

// while loop
while (x > 0) {
    x = x - 1;
}

// print (emits a PTX vprintf call placeholder)
print(x);
```

---

## PTX Output

Each source variable maps to a `.reg .s32` PTX register. Expressions lower to
3-address instructions that mirror real PTX opcodes:

| Source       | PTX                                      |
|--------------|------------------------------------------|
| `a + b`      | `add.s32 %rN, %rA, %rB`                  |
| `a * b`      | `mul.lo.s32 %rN, %rA, %rB`               |
| `a < b`      | `setp.lt.s32 %pN, %rA, %rB`              |
| `if (c)`     | `setp.ne.s32 %p, %r, 0` + `@!%p bra LBL`|
| `while (c)`  | loop top label + predicated `bra`         |
| `!x`         | `setp.eq.s32` + `selp.s32`               |
| `&&` / `\|\|` | `and.pred` / `or.pred` on pred registers |

---

## Compiler Passes

### 1. Lexer (`lexer.h/cpp`)
DFA-based tokeniser. Handles: integer literals, identifiers, keywords (`int if else
while print`), all arithmetic/comparison/logical operators, line and block comments.

### 2. Parser (`parser.h/cpp`)
Recursive-descent parser implementing the grammar:
```
program    -> stmt* EOF
stmt       -> varDecl | ifStmt | whileStmt | printStmt | exprStmt | block
expr       -> assignment | logicOr | logicAnd | equality |
              relational | additive | term | unary | primary
```
Produces a typed AST (`ast.h`) using `std::unique_ptr` for ownership.

### 3. Semantic Analyser (`sema.h/cpp`)
- Scoped symbol table (stack of `unordered_map`)
- Use-before-declaration detection
- Redeclaration-in-same-scope detection
- Walks the full AST in one pass

### 4. Optimiser (`optimizer.h/cpp`)
Constant folding pass (AST-level, pre-codegen):
- **Full fold**: both operands are literals → evaluates at compile time
  - `2 + 3` → `5`, `6 / 2` → `3`, `3 * (2 + 4)` → `18`
- **Identity elimination**: `x + 0` → `x`, `x * 1` → `x`
- **Zero annihilation**: `x * 0` → `0`, `0 * x` → `0`
- **Unary fold**: `-(5)` → `-5`, `!(0)` → `1`
- Reports the number of folds applied

### 5. Code Generator (`codegen.h/cpp`)
- Greedy register allocator: one register per variable + one per temp
- All registers declared upfront as `.reg .s32` (PTX requirement)
- Predicate registers (`.reg .pred`) for control flow
- Control flow via `setp` + predicated `bra` — exactly as real PTX
- Emits `.version 7.0 / .target sm_75` preamble

---

## Build

**Requirements:** g++ ≥ 7 (C++17) or clang++ ≥ 5. No external dependencies.

```bash
# Clone and build
git clone https://github.com/yourusername/toycompiler
cd toycompiler

# With g++ directly:
g++ -std=c++17 -Wall -O2 -Isrc \
    src/main.cpp src/lexer.cpp src/parser.cpp \
    src/sema.cpp src/optimizer.cpp src/codegen.cpp \
    -o toyc

# Or with CMake:
cmake -B build && cmake --build build
```

---

## Usage

```bash
# Compile a .toy source file to PTX
./toyc tests/fib.toy

# Show token stream (useful for debugging the lexer)
./toyc tests/fib.toy --emit-tokens

# Disable optimisations (compare PTX output with/without folding)
./toyc tests/constfold.toy --no-opt
```

---

## Tests

| File                    | Demonstrates                                   |
|-------------------------|------------------------------------------------|
| `tests/fib.toy`         | While loop, arithmetic, multiple variables     |
| `tests/fizzbuzz.toy`    | Nested if/else, `%` operator, `&&` logic       |
| `tests/sumsq.toy`       | Block-scoped variable, nested loop expressions |
| `tests/constfold.toy`   | All constant folding rules (run with/without `--no-opt` to compare PTX) |

```bash
for f in tests/*.toy; do echo "--- $f ---"; ./toyc "$f" > /dev/null && echo "PASS"; done
```

---

## Relation to Real PTX / PTXAS

Real PTX is NVIDIA's virtual ISA — a stable compiler target that sits between
CUDA C/C++ and device-specific SASS machine code. `ptxas` (the PTX assembler) takes
`.ptx` files and performs register allocation, instruction scheduling, and
architecture-specific lowering.

This compiler mimics that front-to-back flow:
- The **lexer/parser** corresponds to PTXAS's front-end parsing of PTX syntax
- The **semantic pass** mirrors type checking and scope validation in real compilers
- The **constant folding** pass is a standard pre-codegen optimisation in LLVM/GCC/PTXAS
- The **PTX output** uses real opcodes (`add.s32`, `setp.lt.s32`, `selp.s32`,
  `@!%p bra`) that are valid PTX 7.0 and could be fed to `ptxas` with a real
  `.visible .func` wrapper

---

## Possible Extensions

- **Dead code elimination** — remove assignments to variables never read
- **SSA construction** — convert to Static Single Assignment form (prerequisite for most modern optimisations)
- **Type system** — add `.f32` float support, mapping to PTX `add.f32` / `mul.f32`
- **Functions** — user-defined functions with PTX `.func` call/ret
- **Real vprintf** — emit actual CUDA device-side `vprintf` calls so the PTX can run on GPU