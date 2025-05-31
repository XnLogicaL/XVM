// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file cstk.h
 * @brief Defines the call stack and call frame structures for function execution.
 * @details The call stack is a runtime structure that manages active function calls during
 * the execution of programs. Each function call is represented by a `CallInfo`,
 * which stores the execution context for that function, including its closure,
 * local variables, and return address. The `CallStack` holds a fixed number of
 * frames, ensuring bounded recursion and stack safety.
 */
#ifndef XVM_CALL_STACK_H
#define XVM_CALL_STACK_H

#include "xvm_common.h"
#include "xvm_closure.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @struct CallInfo
 * @brief Represents a single function invocation's execution context.
 *
 * Each `CallInfo` holds:
 * - A pointer to the `Closure` being executed (compiled function and captured upvalues).
 * - A pointer to a local variables array (`locals`).
 * - The size of the local variables array (`capacity`).
 * - A saved program counter (`pc`) indicating where execution resumes after return.
 *
 * Copy operations are disabled via `XVM_NOCOPY`, but move semantics are allowed via `XVM_IMPLMOVE`.
 */
struct CallInfo {
    bool     protect = false; ///< Protect callframe from errors
    Closure* closure = NULL;  ///< Function closure being invoked.
    Value*   stk_top = NULL;  ///< Stack top when function was called

    const Instruction* pc = NULL; ///< Program counter when function was called
};

} // namespace xvm

/** @} */

#endif
