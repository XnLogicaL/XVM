// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_API_H
#define XVM_API_H

#include "xvm_common.h"
#include "xvm_state.h"

namespace xvm {

void execute(State& state);

void executeStep(State& state, std::optional<Instruction> insn = std::nullopt);

Value& getRegister(State& state, uint16_t reg);

const Value& getRegister(const State& state, uint16_t reg);

void setRegister(State& state, uint16_t reg, Value&& value);

void push(State& state, Value&& value);

void drop(State& state);

void setLocal(State& state, size_t position, Value&& value);

Value& getLocal(State& state, size_t position);

const Value& getLocal(const State& state, size_t position);

Value& getArgument(State& state, size_t offset);

const Value& getArgument(const State& state, size_t offset);

Value& getGlobal(State& state, const char* name);

const Value& getGlobal(const State& state, const char* name);

void setGlobal(State& state, const char* name, const Value& value);

void call(State& state, const Closure& callee, size_t argc);

void pcall(State& state, const Closure& callee, size_t argc);

} // namespace xvm

#endif
