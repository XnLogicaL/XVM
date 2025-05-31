// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file api-impl.h
 * @brief Internal interpreter API implementation
 * @details Contains functions used by the xvm interpreter engine, including stack
 *          operations, value conversion, function calling, error propagation, closure management,
 *          dictionary/array manipulation, and register management.
 */
#ifndef XVM_API_IMPL_H
#define XVM_API_IMPL_H

#include <Common.h>
#include <Interpreter/Opcode.h>
#include <Interpreter/State.h>
#include <Interpreter/TDict.h>
#include <Interpreter/TArray.h>
#include <Interpreter/TClosure.h>

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {


/**
 * @namespace impl
 * @defgroup impl_namespace
 * @{
 */
namespace impl {

constexpr inline uint16_t BACKEND_REGS_START = kRegCount - 1024;
constexpr inline uint16_t BACKEND_REGS_END = kRegCount - 1;

const InstructionData& __getAddressData(const State* state, const Instruction* const pc);

std::string __getFuncSig(const Callable& func);

/**
 * @brief Sets the interpreter into an error state with a given message.
 * @param state Interpreter state.
 * @param message The error message.
 */
void __ethrow(State* state, const std::string& message);

template<typename... Args>
void __ethrowf(State* state, const std::string& fmt, Args&&... args) {
    try {
        auto str = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));
        __ethrow(state, str);
    }
    catch (const std::format_error&) {
        // This function is the only exception to non-assertive implementation functions.
        XVM_ASSERT(false, "__ethrowf: Invalid format string")
    }
}

/**
 * @brief Clears any existing error state in the interpreter.
 * @param state Interpreter state.
 */
void __eclear(State* state);

/**
 * @brief Checks whether the interpreter is currently in an error state.
 * @param state Interpreter state.
 * @return true if an error is currently set; false otherwise.
 */
bool __echeck(const State* state);

/**
 * @brief Handles a currently active error by unwinding the call stack.
 * @details Will terminate the program if no handler is found.
 * @param state Interpreter state.
 * @return true if the error was successfully handled; false otherwise.
 */
bool __ehandle(State* state);

/**
 * @brief Retrieves a constant value from the constant pool.
 * @param state Interpreter state.
 * @param index Index of the constant.
 * @return The constant value.
 */
Value __getConstant(const State* state, size_t index);

/**
 * @brief Returns the type of a value as a xvm string object.
 * @param val The value to inspect.
 * @return Type as an std::string
 */
std::string __type(const Value& val);

/**
 * @brief Gets the raw pointer stored in a value, or NULL if not applicable.
 * @param val The value to inspect.
 * @return The underlying pointer.
 */
void* __toPointer(const Value& val);

/**
 * @brief Pushes a new call frame onto the call stack.
 * @param state Interpreter state.
 * @param frame Call frame to push.
 */
void __cipush(State* state, CallInfo&& ci);

/**
 * @brief Pops the topmost call frame from the stack.
 * @param state Interpreter state.
 */
void __cipop(State* state);

/**
 * @brief Calls a function using a dynamic dispatch system.
 * @details Supports native and external function types.
 * @param state Interpreter state.
 * @param callee Function to call.
 */
void __call(State* state, Closure* callee);

/**
 * @brief Calls a function in a protected manner using a dynamic dispatch system.
 * @details Supports native and external function types.
 * @param state Interpreter state.
 * @param callee Function to call.
 */
void __pcall(State* state, Closure* callee);

/**
 * @brief Performs a return from a function.
 * @details Restores the previous call frame and pushes a return value.
 * @param state Interpreter state.
 * @param retv Value to return.
 */
void __return(State* XVM_RESTRICT state, Value&& retv);

/**
 * @brief Returns the length of the given value as a Value object.
 *
 * Supports arrays, strings, and dictionaries. Returns Nil if length cannot be determined.
 *
 * @param val The value to get the length of.
 * @return Value Length as an `int`, -1 if impossible.
 */
int __length(const Value& val);

/**
 * @brief Converts the given value to a language-level String object.
 *
 * Uses the value's stringification rules defined by the VM or language runtime.
 *
 * @param val The value to convert.
 * @return Value The value as an std::string.
 */
std::string __toString(const Value& val);

/**
 * @brief Converts a value to its boolean representation.
 *
 * Applies standard truthiness rules:
 * - false and nil are false,
 * - everything else is true.
 *
 * @param val The value to evaluate.
 * @return Value A boolean Value representing the truthiness.
 */
bool __toBool(const Value& val);

/**
 * @brief Converts the given value to an integer Value or returns Nil if not possible.
 *
 * Attempts string-to-int and float-to-int conversion if applicable.
 *
 * @param V The VM state (used for allocations or error tracking).
 * @param val The value to convert.
 * @return Value Integer Value or Nil.
 */
Value __toInt(State* state, const Value& val);

/**
 * @brief Converts the given value to a floating-point Value or returns Nil if not possible.
 *
 * Performs conversion from strings or integers as needed.
 *
 * @param V The VM state (used for allocations or error tracking).
 * @param val The value to convert.
 * @return Value Float Value or Nil.
 */
Value __toFloat(State* state, const Value& val);

/**
 * @brief Deeply compares two values for equality.
 *
 * Handles primitive types and recursively compares structures like arrays and dictionaries.
 *
 * @param val0 First value.
 * @param val1 Second value.
 * @return bool True if values are shallow-ly equal, false otherwise.
 */
bool __compare(const Value& val0, const Value& val1);

/**
 * @brief Deeply compares two values for equality.
 *
 * Handles primitive types and recursively compares structures like arrays and dictionaries.
 *
 * @param val0 First value.
 * @param val1 Second value.
 * @return bool True if values are deeply equal, false otherwise.
 */
bool __compareDeep(const Value& val0, const Value& val1);

// Automatically resizes UpValue vector of closure by XVM_UPV_RESIZE_FACTOR.
void __resizeClosureUpvs(Closure* closure);

// Checks if a given index is within the bounds of the UpValue vector of the closure.
// Used for resizing.
bool __rangeCheckClosureUpvs(Closure* closure, size_t index);

// Attempts to retrieve UpValue at index <upv_id>.
// Returns NULL if <upv_id> is out of UpValue vector bounds.
UpValue* __getClosureUpv(Closure* closure, size_t upv_id);

/**
 * @brief Resizes the UpValue vector of the given closure.
 * @note The vector is grown by a constant `XVM_UPV_RESIZE_FACTOR`.
 * @param closure Pointer to the closure.
 */
void __resizeClosureUpvs(Closure* closure);

/**
 * @brief Checks if the given index is within bounds of the closure's UpValue vector.
 *
 * @param closure Pointer to the closure.
 * @param index Index to validate.
 * @return bool True if within range, false otherwise.
 */
bool __rangeCheckClosureUpvs(Closure* closure, size_t index);

/**
 * @brief Gets the UpValue at the specified index.
 *
 * @param closure Pointer to the closure.
 * @param upv_id Index of the UpValue.
 * @return UpValue* Pointer to UpValue or NULL if out of bounds.
 */
UpValue* __getClosureUpv(Closure* closure, size_t upv_id);

/**
 * @brief Sets the UpValue at the specified index to a given value.
 *
 * @param closure Pointer to the closure.
 * @param upv_id Index of the UpValue.
 * @param val Value to set.
 */
void __setClosureUpv(Closure* closure, size_t upv_id, Value& val);

/**
 * @brief Loads bytecode instructions into the closure.
 *
 * Handles special opcodes like RET and CAPTURE during loading.
 *
 * @param state Runtime state.
 * @param closure Target closure.
 * @param len Length of bytecode stream.
 */
void __initClosure(State* state, Closure* closure, size_t len);

/**
 * @brief Closes the closure’s upvalues and moves them to the heap.
 *
 * Typically used at function return to preserve captured variables.
 *
 * @param closure The closure whose upvalues to close.
 */
void __closeClosureUpvs(const Closure* closure);

/**
 * @brief Hashes a key string using FNV-1a.
 *
 * @param dict Dictionary context.
 * @param key Null-terminated string key.
 * @return size_t Hash value.
 */
size_t __hashDictKey(const Dict* dict, const char* key);

/**
 * @brief Sets a key-value pair in the dictionary.
 *
 * Updates existing key if already present.
 *
 * @param dict Pointer to dictionary.
 * @param key Null-terminated key.
 * @param val Value to insert.
 */
void __setDictField(const Dict* dict, const char* key, Value val);

/**
 * @brief Retrieves the value associated with a key.
 *
 * @param dict Pointer to dictionary.
 * @param key Null-terminated key string.
 * @return Value* Pointer to value or NULL if key not found.
 */
Value* __getDictField(const Dict* dict, const char* key);

/**
 * @brief Returns number of entries in the dictionary.
 *
 * @param dict Pointer to dictionary.
 * @return size_t Number of key-value pairs.
 */
size_t __getDictSize(const Dict* dict);

/**
 * @brief Checks if an index is valid in the array.
 *
 * @param array Pointer to array.
 * @param index Index to check.
 * @return bool True if valid, false otherwise.
 */
bool __rangeCheckArray(const Array* array, size_t index);

/**
 * @brief Resizes the array component of the table.
 *
 * Grows capacity to accommodate more elements.
 *
 * @param array Pointer to array.
 */
void __resizeArray(Array* array);

/**
 * @brief Sets a value at a specific index in the array.
 *
 * Resizes if the index exceeds current capacity.
 *
 * @param array Pointer to array.
 * @param index Index to assign.
 * @param val Value to assign.
 */
void __setArrayField(Array* array, size_t index, Value val);

/**
 * @brief Retrieves a value at a specific index.
 *
 * @param array Pointer to array.
 * @param index Index to read.
 * @return Value* Pointer to value or NULL if out of bounds.
 */
Value* __getArrayField(const Array* array, size_t index);

/**
 * @brief Returns the current size (number of used elements) of the array.
 *
 * @param array Pointer to array.
 * @return size_t Array size.
 */
size_t __getArraySize(const Array* array);

void __setString(String* string, size_t index, char chr);
char __getString(String* string, size_t index);

String* __concatString(String* left, String* right);

/**
 * @brief Returns a pointer to a label instruction by index.
 *
 * @param state The runtime state.
 * @param index Index of label.
 * @return Instruction* Pointer to instruction or NULL.
 */
const Instruction* __getLabelAddress(const State* state, size_t index);

/**
 * @brief Pushes a value onto the VM stack.
 *
 * @param state The runtime state.
 * @param val Value to push.
 */
void __push(State* state, Value&& val);

/**
 * @brief Drops the top value from the VM stack.
 *
 * @param state The runtime state.
 */
void __drop(State* state);

/**
 * @brief Retrieves a local variable at a given offset.
 *
 * @param state The runtime state.
 * @param offset Stack frame offset.
 * @return Value* Pointer to local value.
 */
StkId __getLocal(State* XVM_RESTRICT state, size_t offset);

const StkId __getLocal(const State* XVM_RESTRICT state, size_t offset);

/**
 * @brief Sets a local variable at a given offset.
 *
 * @param state The runtime state.
 * @param offset Stack frame offset.
 * @param val Value to set.
 */
void __setLocal(State* XVM_RESTRICT state, size_t offset, Value&& val);

/**
 * @brief Assigns a value to a register.
 *
 * @param state The runtime state.
 * @param reg Register index.
 * @param val Value to assign.
 */
void __setRegister(State* state, uint16_t reg, Value&& val);

/**
 * @brief Retrieves a value from a register.
 *
 * @param state The runtime state.
 * @param reg Register index.
 * @return Value* Pointer to register value.
 */
Value* __getRegister(State* state, uint16_t reg);

const Value* __getRegister(const State* state, uint16_t reg);

} // namespace impl

/** @} */

} // namespace xvm

/** @} */

#endif
