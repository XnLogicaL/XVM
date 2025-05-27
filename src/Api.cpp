// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>
#include <cmath>

namespace xvm {

using enum ValueKind;

Value& State::get_register(uint16_t reg) {
    return *impl::__get_register(this, reg);
}

void State::set_register(uint16_t reg, Value val) {
    impl::__set_register(this, reg, std::move(val));
}

void State::push(Value&& val) {
    CallFrame* frame = impl::__current_callframe(this);
    XVM_ASSERT(frame->capacity <= CALLFRAME_MAX_LOCALS, "stack overflow");
    impl::__push(this, std::move(val));
}

void State::drop() {
    CallFrame* frame = impl::__current_callframe(this);
    XVM_ASSERT(frame->capacity > 0, "stack underflow");
    impl::__drop(this);
}

size_t State::stack_size() {
    CallFrame* current_callframe = impl::__current_callframe(this);
    return current_callframe->capacity;
}

Value& State::get_global(const char* name) {
    return globals->get(name);
}

} // namespace xvm
