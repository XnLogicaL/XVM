// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/Callstack.h>

namespace xvm {

CallFrame::CallFrame()
  : locals(new Value[CALLFRAME_MAX_LOCALS]) {}

CallFrame::CallFrame(CallFrame&& other)
  : closure(other.closure),
    locals(other.locals),
    savedpc(other.savedpc) {
    other.closure = NULL;
    other.locals = NULL;
    other.savedpc = NULL;
}

CallFrame& CallFrame::operator=(CallFrame&& other) {
    if (this != &other) {
        delete[] this->locals;

        this->closure = other.closure;
        this->locals = other.locals;
        this->savedpc = other.savedpc;

        other.closure = NULL;
        other.locals = NULL;
        other.savedpc = NULL;
    }

    return *this;
}

CallFrame::~CallFrame() {
    delete closure;
    delete[] locals;
}

} // namespace xvm
