// This file is a part of the XVM Project
// Copyright (C) 2024-2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file Container.h
 */
#ifndef XVM_CONTAINER_H
#define XVM_CONTAINER_H

#include "xvm_common.h"
#include "xvm_instruction.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @class OperandsArray
 * @brief std::array wrapper with custom initialization support
 */
struct OperandsArray {
    std::array<uint16_t, 3> data;

    inline constexpr OperandsArray() {
        data.fill(OPERAND_INVALID);
    }

    inline constexpr OperandsArray(std::initializer_list<uint16_t> init) {
        data.fill(OPERAND_INVALID);
        std::copy(init.begin(), init.end(), data.begin());
    }

    inline operator std::array<uint16_t, 3>&() {
        return data;
    }

    inline operator const std::array<uint16_t, 3>&() const {
        return data;
    }
};

/**
 * @struct BytecodeHolder
 * @brief Lightweight container that is used to store instructions and their data.
 */
struct BytecodeHolder {
    std::vector<Instruction>     insns; ///< Instruction array
    std::vector<InstructionData> data;  ///< Instruction data array

    /**
     * @brief Pushes a raw instruction and its raw data into their respective containers
     * @param insn Instruction
     * @param data Instruction data
     */
    void pushInstructionRaw(Instruction&& insn, InstructionData&& data);

    /**
     * @brief Constructs and pushes an instruction and its data into their respective containers
     * @param op Opcode of the instruction
     * @param ops Operands of the instruction
     * @param comment Comment of the instruction
     */
    void pushInstruction(Opcode op, OperandsArray&& ops, std::string&& comment);
};

} // namespace xvm

#endif
