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
    main(Value(__initMainFunction())),
    stack_registers(stk_registers) {
    __initRegisterFile(this);
    __initLabelAddressTable(this, 0 /* TODO */);
    __call(this, main.u.clsr);
    __linearScanLabelsInBytecode(this);
    __initCoreInterpLib(this);
}

State::~State() {
    delete globals;
    delete callstack;
    delete err;

    __deinitRegisterFile(this);
    __deinitLabelAddressTable(this);
}

} // namespace xvm
