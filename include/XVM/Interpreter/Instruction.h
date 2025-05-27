// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file instruction.h
 * @brief Defines the representation of bytecode instructions in the xvm VM.
 *
 * This file provides the core structures and type definitions used to represent
 * low-level bytecode instructions that the virtual machine executes. It includes
 * operand types, the main `Instruction` structure, and associated metadata for debugging.
 */
#ifndef XVM_INSTRUCTION_H
#define XVM_INSTRUCTION_H

#include <Common.h>
#include <Interpreter/Opcode.h>
#include <cstdint>

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @brief Sentinel value used to represent an invalid or unused operand.
 */
inline constexpr size_t OPERAND_INVALID = 0xFFFF;

/**
 * @struct InstructionData
 * @brief Optional debug metadata associated with a single instruction.
 *
 * Currently only includes a comment string, but may be extended in the future
 * to support source mapping, line numbers, or debugging flags.
 */
struct InstructionData {
    std::string comment = ""; ///< Human-readable comment or annotation.
};

/**
 * @struct Instruction
 * @brief Represents a single VM instruction in the xvm bytecode format.
 *
 * Each instruction has:
 * - An opcode (`op`) that specifies the operation to perform.
 * - Up to three 16-bit operands (`a`, `b`, and `c`), whose semantics depend on the opcode.
 *
 * The structure is aligned to 8 bytes to optimize for memory layout and access efficiency.
 */
struct alignas(8) Instruction {
    Opcode   op = Opcode::NOP;     ///< Operation code (e.g., ADD, LOAD, CALL).
    uint16_t a  = OPERAND_INVALID; ///< First operand (typically a register or constant index).
    uint16_t b  = OPERAND_INVALID; ///< Second operand.
    uint16_t c  = OPERAND_INVALID; ///< Third operand.
};

} // namespace xvm

/** @} */

#endif
