// This file is a part of the XVM Project
// Copyright (C) 2024-2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_container.h"

namespace xvm {

void BytecodeHolder::pushInstructionRaw(Instruction&& insn, InstructionData&& data) {
    this->insns.push_back(insn);
    this->data.push_back(data);
}

void BytecodeHolder::pushInstruction(Opcode op, OperandsArray&& ops, std::string&& comment) {
    Instruction     insn{op, ops.data[0], ops.data[1], ops.data[2]};
    InstructionData data{comment};

    this->insns.push_back(std::move(insn));
    this->data.push_back(std::move(data));
}

const Instruction* BytecodeHolder::getCode() const {
    return insns.data();
}

const InstructionData& BytecodeHolder::getData(size_t pos) const {
    return data.at(pos);
}

size_t BytecodeHolder::getCodeSize() const {
    return insns.size();
}

void ConstantHolder::pushConstant(Value&& val) {
    constants.push_back(std::move(val));
}

const Value& ConstantHolder::getConstant(size_t pos) const {
    static Value nil = XVM_NIL;

    if (pos >= constants.size())
        return nil;

    return constants.at(pos);
}

} // namespace xvm
