// This file is a part of the XVM Project
// Copyright (C) 2024-2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/Container.h>

namespace xvm {

void BytecodeHolder::pushInstructionRaw(Instruction&& insn, InstructionData&& data) {
    this->insns.push_back(insn);
    this->data.push_back(data);
}

void BytecodeHolder::pushInstruction(Opcode op, OperandsArray&& ops, std::string&& comment) {
    Instruction     insn{op, ops.data[0], ops.data[1], ops.data[2]};
    InstructionData data{comment};
    pushInstructionRaw(std::move(insn), std::move(data));
}

} // namespace xvm
