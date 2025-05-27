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

#include <Common.h>
#include <Interpreter/Callstack.h>
#include <Interpreter/Instruction.h>
#include <Interpreter/TValue.h>

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @brief Total number of general-purpose registers available.
 * Matches the operand range (2^16).
 */
inline constexpr size_t REGISTER_COUNT = 0xFFFF + 1;

/**
 * @brief Number of registers reserved for stack-based usage.
 * These are backed by fast C++ stack memory.
 */
inline constexpr size_t REGISTER_STACK_COUNT = 0x00FF + 1;

/**
 * @brief Number of registers reserved for heap-based spilling.
 * These are allocated separately and used when stack registers are exhausted.
 */
inline constexpr size_t REGISTER_SPILL_COUNT = REGISTER_COUNT - REGISTER_STACK_COUNT;

/**
 * @struct ErrorState
 * @brief Represents an active runtime error during VM execution.
 */
struct ErrorState {
    bool        has_error = false;
    std::string funcsig = ""; ///< Function signature of where the error occurred.
    std::string message = ""; ///< Human-readable error message.
};

/**
 * @brief Generic register array wrapper with fixed size and alignment.
 * @tparam Size The number of registers in the holder.
 */
template<const size_t Size>
struct alignas(64) RegFile {
    Value registers[Size];
};

/// Type alias for stack-based register block.
using StkRegFile = RegFile<REGISTER_STACK_COUNT>;

/// Type alias for heap/spill register block.
using SpillRegFile = RegFile<REGISTER_SPILL_COUNT>;

/**
 * @class State
 * @brief Represents the complete virtual machine execution state.
 *
 * This object owns and manages the program counter, call stack, register file,
 * globals, error reporting, and runtime execution loop. The structure is aligned
 * to 64 bytes to ensure optimal CPU cache usage during high-frequency access.
 */
struct alignas(64) State {
    Instruction*  pc = NULL;     ///< Current instruction pointer.
    Instruction** labels = NULL; ///< Jump target label array.

    Dict*       globals = NULL;   ///< Global variable dictionary.
    CallStack*  callstack = NULL; ///< Call stack of function frames.
    ErrorState* err = NULL;       ///< Active/inactive error state.

    Value      main = Value(); ///< Reference to the main function.
    register_t ret = NULL;     ///< Return register index.
    register_t args = NULL;    ///< First argument register index.

    StkRegFile&   stack_registers;        ///< Stack register block.
    SpillRegFile* spill_registers = NULL; ///< Heap (spill) register block.

    XVM_NOCOPY(State); ///< The state object should not be copied.
    XVM_NOMOVE(State); ///< The state object should not be moved.

    /**
     * @brief Constructs a new State.
     * @param stack_regs Reference to the stack register block.
     * @param lctx Reference to the translation unit's compilation context.
     */
    explicit State(StkRegFile& stack_regs);
    ~State();

    /// Begins executing instructions starting at the current program counter (`pc`).
    void execute();

    /// Executes a single instruction step. Optionally provide an override instruction.
    void execute_step(std::optional<Instruction> insn = std::nullopt);

    /// Returns a mutable reference to the value stored in the given register.
    Value& get_register(uint16_t reg);

    /// Assigns a value to the given register.
    void set_register(uint16_t reg, Value value);

    /// Pushes a generic value onto the stack.
    void push(Value&& value);

    /// Drops the top value from the stack and frees its resources.
    void drop();

    /// Assigns a value to a local variable at the given position.
    void set_local(size_t position, Value value);

    /// Returns a reference to a local value at the given position.
    Value& get_local(size_t position);

    /// Returns a reference to an argument value relative to the current call frame.
    Value& get_argument(size_t offset);

    /// Returns the current size of the value stack.
    size_t stack_size();

    /// Retrieves the global variable with the given name.
    Value& get_global(const char* name);

    /// Assigns a value to a global variable with the given name.
    void set_global(const char* name, const Value& value);

    /// Invokes a function closure with a given number of arguments.
    void call(const Closure& callee, size_t argc);
};

} // namespace xvm

/** @} */

#endif
