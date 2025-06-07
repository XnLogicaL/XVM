// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file state.h
 * @brief Declares the State class and related components for managing VM execution.
 *
 * The State object encapsulates the full runtime state of the virtual machine.
 * It holds the register space (stack and heap), manages the call stack, and controls
 * instruction dispatch and execution. It provides an interface for interacting with
 * local, global, and argument values, as well as for calling functions and handling errors.
 */
#ifndef XVM_STATE_H
#define XVM_STATE_H

#include "xvm_common.h"
#include "xvm_callinfo.h"
#include "xvm_instruction.h"
#include "xvm_value.h"
#include "xvm_allocator.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/// Total amount of addressable registers (2^16)
constexpr XVM_GLOBAL size_t kRegCount = 0xFFFF + 1;

constexpr XVM_GLOBAL size_t kMaxLocalCount = 200;

constexpr XVM_GLOBAL size_t kMaxCiCount = 200;

constexpr XVM_GLOBAL size_t kStrAllocPoolSize = 256 * 1024;

using StkId = Value*;
using CiStkId = CallInfo*;

/**
 * @struct ErrorInfo
 * @brief Represents an active runtime error during VM execution.
 */
struct ErrorInfo {
  bool error = false;

  const char* func; ///< Function signature of where the error occurred.
  const char* msg;  ///< Human-readable error message.
};

/**
 * @class State
 * @brief Represents the complete virtual machine execution state.
 *
 * This object owns and manages the program counter, call stack, register file,
 * genv, error reporting, and runtime execution loop. The structure is aligned
 * to 64 bytes to ensure optimal CPU cache usage during high-frequency access.
 */
// clang-format off
struct alignas(64) State {
    const std::vector<Value>& k_holder;                 ///< Constant array
    const std::vector<Instruction>& bc_holder;          ///< Bytecode array
    const std::vector<InstructionData>& bc_info_holder;    

    Dict* genv = NULL;                                  ///< Global environment

    TempBuf<Value> regs{kRegCount};
    TempObj<ErrorInfo> einfo;                           ///< Error info
    TempBuf<Value> stk{kMaxLocalCount};                 ///< Stack base
    TempBuf<CallInfo> cis{kMaxCiCount};                 ///< Call info stack

    StkId stk_top = NULL;                               ///< Top of the stack
    StkId stk_base = NULL;                              ///< Base of the current function
    CiStkId ci_top = NULL;                              ///< Top of the callinfo stack
    const Instruction* pc = NULL;                       ///< Program counter

    Value main = XVM_NIL;                               ///< Main function slot

    ByteAllocator<char> salloc{kStrAllocPoolSize};      ///< String arena allocator

    // The state object is not intended to be copied or moved to other locations in memory.
    // It should only be tied to a larger global state, and should be passed around as references.
    XVM_NOCOPY(State);
    XVM_NOMOVE(State);

    // State objects are tied to other objects like bytecode containers and register files,
    // therefore are not default constructible.
    State() = delete;

    explicit State(
        const std::vector<Value>& k_holder,
        const std::vector<Instruction>& bc_holder,
        const std::vector<InstructionData>& bc_info_holder
    );

    ~State();
};
// clang-format on

} // namespace xvm

/** @} */

#endif
