// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/Callstack.h>

namespace xvm {

CallInfo::CallInfo()
  : locals(new Value[CALLFRAME_MAX_LOCALS]) {}

CallInfo::CallInfo(CallInfo&& other)
  : closure(other.closure),
    locals(other.locals),
    pc(other.pc) {
    other.closure = NULL;
    other.locals = NULL;
    other.pc = NULL;
}

CallInfo& CallInfo::operator=(CallInfo&& other) {
    if (this != &other) {
        delete[] this->locals;

        this->closure = other.closure;
        this->locals = other.locals;
        this->pc = other.pc;

        other.closure = NULL;
        other.locals = NULL;
        other.pc = NULL;
    }

    return *this;
}

CallInfo::~CallInfo() {
    delete closure;
    delete[] locals;
}

} // namespace xvm
