// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

State::State(BytecodeHolder& holder, size_t labelCount, StkRegFile& regs)
  : lat(labelCount),
    globals(new Dict),
    stkRegs(regs),
    holder(holder),
    strAlloc(1024 * 256) {
    impl::__initRegisterFile(this);
    impl::__linearScanLabelsInBytecode(this);
    impl::__initCoreInterpLib(this);
    impl::__initMainFunction(this);
    // Call main
    impl::__call(this, main.u.clsr);
}

State::~State() {
    delete globals;
    impl::__deinitRegisterFile(this);
}

} // namespace xvm
