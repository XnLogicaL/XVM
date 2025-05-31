// This file is a part of the XVM Project
// Copyright (C) 2024-2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file Container.h
 */
#ifndef XVM_CONTAINER_H
#define XVM_CONTAINER_H

#include "xvm_common.h"
#include "xvm_value.h"
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
class BytecodeHolder {
  public:
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

    const Instruction*     getCode() const;
    const InstructionData& getData(size_t pos) const;

    size_t getCodeSize() const;

  private:
    std::vector<Instruction>     insns; ///< Instruction array
    std::vector<InstructionData> data;  ///< Instruction data array
};

class ConstantHolder {
  public:
    void pushConstant(Value&& val);

    template<typename... Args>
        requires std::is_constructible_v<Value, Args...>
    void pushConstant(Args&&... args) {
        return pushConstant(Value(args...));
    }

    const Value& getConstant(size_t pos) const;

  private:
    std::vector<Value> constants;
};

} // namespace xvm

#endif
