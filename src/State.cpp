// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

using namespace impl;

// Initializes and returns a new state object
State::State(StkRegFile& stk_registers)
  : globals(new Dict),
    callstack(new CallStack),
    err(new ErrorState),
    main(Value(__create_main_function())),
    stack_registers(stk_registers) {
    __register_allocate(this);
    __label_allocate(this, 0 /* TODO */);
    __call(this, main.u.clsr);
    __label_load(this);
    __declare_core_lib(this);
}

State::~State() {
    delete globals;
    delete callstack;
    delete err;

    __register_deallocate(this);
    __label_deallocate(this);
}

} // namespace xvm
