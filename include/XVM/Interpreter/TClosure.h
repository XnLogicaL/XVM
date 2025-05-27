// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file tfunction.h
 * @brief Declares function, closure, and upvalue types used for virtual machine function
 * invocation.
 *
 * This includes both user-defined and native function representations, along with closures and
 * upvalue capture logic for supporting lexical scoping and first-class functions.
 */
#ifndef XVM_FUNCTION_H
#define XVM_FUNCTION_H

#include <Common.h>
#include <Interpreter/Instruction.h>
#include <Interpreter/TValue.h>

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

inline constexpr size_t kClosureUpvCount = 10; ///< Default number of upvalues reserved.

struct State;
struct Closure;
struct CallFrame;

/**
 * @struct UpValue
 * @brief Represents a captured variable in a closure.
 *
 * An UpValue can either point directly to a value still on the stack (open),
 * or contain a heap-allocated copy of the value (closed).
 */
struct UpValue {
    bool   open = true;    ///< Whether the upvalue is open (points to stack).
    bool   valid = false;  ///< Whether the upvalue has been properly initialized.
    Value* value = NULL;   ///< Pointer to the actual value, or null.
    Value  heap = XVM_NIL; ///< Used to store the value when closed.
};

/**
 * @struct Function
 * @brief Represents a user-defined xvm function, including its bytecode and metadata.
 */
struct Function {
    const char*  id = "<anonymous>"; ///< Identifier string or default name.
    size_t       line = 0;           ///< Line number where function was defined (for debugging).
    size_t       size = 0;           ///< Total number of instructions.
    Instruction* code = NULL;        ///< Pointer to the function’s instruction sequence.
};

/**
 * @brief Type alias for native C++ functions that can be called by the VM.
 * They receive a pointer to the current interpreter state and return a `Value`.
 */
using NativeFn = Value (*)(State* interpreter);

enum class CallableKind {
    Function, ///< User-defined function.
    Native,   ///< Native function.
};

/**
 * @struct Callable
 * @brief Wraps a function-like object, either user-defined or native.
 *
 * Used uniformly throughout the VM for calling both compiled and native routines.
 */
struct Callable {
    size_t       arity = 0; ///< Number of arguments expected.
    CallableKind type = CallableKind::Native;

    /**
     * @union Un
     * @brief Stores either a pointer to a `Function` or a `NativeFn`.
     */
    union {
        Function fn;
        NativeFn ntv;
    } u = {NULL};

    Callable() = default;
};

/**
 * @struct Closure
 * @brief Wraps a Callable with its captured upvalues for lexical scoping.
 *
 * A Closure is created when a function expression references non-local variables.
 */
struct Closure {
    Callable callee = {}; ///< Underlying callable (function or native).
    UpValue* upvs = NULL; ///< Array of upvalue pointers.
    size_t   count = 0;   ///< Number of captured upvalues.

    XVM_IMPLCOPY(Closure);
    XVM_IMPLMOVE(Closure);

    Closure();

    /**
     * @brief Constructs a closure from a callable (usually a function).
     * @param callable The callable to close over.
     */
    Closure(Callable&& callable);

    /**
     * @brief Destructor that frees upvalue array.
     */
    ~Closure();
};

} // namespace xvm

/** @} */

#endif
