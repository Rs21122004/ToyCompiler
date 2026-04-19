#pragma once
#include "ast.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

// ─── PTX-style code generator ─────────────────────────────────────────────────
//
// Emits a subset of real PTX assembly (NVIDIA's virtual ISA).
// Every variable maps to a .s32 register (%r0, %r1, ...).
// Expressions are reduced to 3-address instructions.
// Control flow uses setp / @p bra (real PTX style).
//
// Output is valid-looking PTX that could be fed to ptxas after adding
// a .version / .target / .visible .func wrapper.

class Codegen {
public:
    std::string generate(const std::vector<StmtPtr>& program);

private:
    std::ostringstream out_;

    // Register allocation
    int                                    regCounter_   = 0;
    int                                    labelCounter_ = 0;
    std::unordered_map<std::string, std::string> varToReg_;  // source var -> PTX reg name
    std::vector<std::string>               declaredRegs_;

    // Declare/lookup registers
    std::string newReg();
    std::string varReg(const std::string& name);

    // Label generation
    std::string newLabel(const std::string& prefix);

    // Emission helpers
    void emit(const std::string& instr);
    void emitLabel(const std::string& lbl);

    // Code generation passes
    void genStmt(const StmtNode& s);

    // Returns the PTX register holding the result of evaluating e
    std::string genExpr(const ExprNode& e);
};