// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>
#include <cmath>

namespace xvm {

using enum ValueKind;

Value& getRegister(State& state, uint16_t reg) {
    return *impl::__getRegister(&state, reg);
}

const Value& getRegister(const State& state, uint16_t reg) {
    return *impl::__getRegister(&state, reg);
}

void setRegister(State& state, uint16_t reg, Value&& val) {
    impl::__setRegister(&state, reg, std::move(val));
}

void push(State& state, Value&& val) {
    CallInfo* ci = state.ci_top - 1;
    XVM_ASSERT(state.cis.data - ci >= kMaxLocalCount, "stack overflow");
    impl::__push(&state, std::move(val));
}

void drop(State& state) {
    CallInfo* ci = state.ci_top - 1;
    XVM_ASSERT(ci == state.cis.data, "stack underflow");
    impl::__drop(&state);
}

void setLocal(State& state, size_t position, Value&& value) {
    return impl::__setLocal(&state, position, std::move(value));
}

Value& getLocal(State& state, size_t position) {
    return *impl::__getLocal(&state, position);
}

const Value& getLocal(const State& state, size_t position) {
    return *impl::__getLocal(&state, position);
}

Value& getArgument(State& state, size_t offset) {
    return *(state.stk_base + offset - 1);
}

const Value& getArgument(const State& state, size_t offset) {
    return *(state.stk_base + offset - 1);
}

Value& getGlobal(State& state, const char* name) {
    return state.genv->get(name);
}

const Value& getGlobal(const State& state, const char* name) {
    return state.genv->get(name);
}

} // namespace xvm
