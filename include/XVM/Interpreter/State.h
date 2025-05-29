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
#include <Interpreter/Allocator.h>
#include <Interpreter/Container.h>

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

inline constexpr size_t kRegCount = 0xFFFF + 1;    ///< Total amount of addressable registers (2^16)
inline constexpr size_t kStkRegCount = 0x00FF + 1; ///< Amount of stack registers (2^8)
inline constexpr size_t kHeapRegCount = kRegCount - kStkRegCount; /// Amount of heap registers

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
using StkRegFile = RegFile<kStkRegCount>;

/// Type alias for heap/spill register block.
using SpillRegFile = RegFile<kHeapRegCount>;

/**
 * @class State
 * @brief Represents the complete virtual machine execution state.
 *
 * This object owns and manages the program counter, call stack, register file,
 * globals, error reporting, and runtime execution loop. The structure is aligned
 * to 64 bytes to ensure optimal CPU cache usage during high-frequency access.
 */
struct alignas(64) State {
    BytecodeHolder& holder; ///< Reference to bytecode array

    struct Dict* globals = NULL; ///< Global variable dictionary. Not wrapped in TempObj because it
                                 ///< is not defined locally.

    Instruction* pc = NULL; ///< Program counter.

    TempBuf<Instruction*> laddress_table; ///< Label address table.
    TempObj<CallStack>    callstack;      ///< Call stack of function frames.
    TempObj<ErrorState>   error_info;     ///< Active/inactive error state.

    Value    main = XVM_NIL; ///< Reference to the main function.
    uint16_t ret = 0;        ///< Return register index.
    uint16_t args = 0;       ///< First argument register index.

    StkRegFile&           stk_regf;  ///< Stack register file.
    TempObj<SpillRegFile> heap_regf; ///< Heap (spill) register file.

    ByteAllocator str_alloc; ///< String arena allocator.

    XVM_NOCOPY(State); ///< The state object should not be copied.
    XVM_NOMOVE(State); ///< The state object should not be moved.

    /**
     * @brief Constructs a new State.
     * @param file Reference to the stack register file.
     */
    State(BytecodeHolder& holder, StkRegFile& file);
    ~State();

    /// Begins executing instructions starting at the current program counter (`pc`).
    void execute();

    /// Executes a single instruction step. Optionally provide an override instruction.
    void executeStep(std::optional<Instruction> insn = std::nullopt);

    /// Returns a mutable reference to the value stored in the given register.
    Value& getRegister(uint16_t reg);

    /// Assigns a value to the given register.
    void setRegister(uint16_t reg, Value value);

    /// Pushes a generic value onto the stack.
    void push(Value&& value);

    /// Drops the top value from the stack and frees its resources.
    void drop();

    /// Assigns a value to a local variable at the given position.
    void setLocal(size_t position, Value value);

    /// Returns a reference to a local value at the given position.
    Value& getLocal(size_t position);

    /// Returns a reference to an argument value relative to the current call frame.
    Value& getArg(size_t offset);

    /// Retrieves the global variable with the given name.
    Value& getGlobal(const char* name);

    /// Assigns a value to a global variable with the given name.
    void setGlobal(const char* name, const Value& value);

    /// Invokes a function closure with a given number of arguments.
    void call(const Closure& callee, size_t argc);
};

} // namespace xvm

/** @} */

#endif
