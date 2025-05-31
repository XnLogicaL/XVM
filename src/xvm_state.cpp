// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_state.h"
#include "xvm_api_impl.h"
#include "xvm_lib_base.h"

namespace xvm {

using enum Opcode;

static size_t countLabels(BytecodeHolder& holder) {
    size_t lcount = 0;

    for (const Instruction& insn : holder.insns) {
        if (insn.op == LBL)
            lcount++;
    }

    return lcount;
}

static void loadLabels(const State* state) {
    size_t lindex = 0;

    const Instruction* pcbase = state->holder.insns.data();

    for (size_t counter = 0; const Instruction& insn : state->holder.insns) {
        if (insn.op == LBL)
            state->lat.data[lindex++] = pcbase + counter;
        ++counter;
    }
}

static void loadMainFunction(State* state) {
    Function fun;
    fun.id = "main";
    fun.line = 0;
    fun.code = state->holder.insns.data();
    fun.size = state->holder.insns.size();

    Callable c;
    c.type = CallableKind::Function;
    c.u = {.fn = fun};
    c.arity = 1;

    Closure* cl = new Closure(std::move(c));
    state->main = Value(cl);
}

State::State(BytecodeHolder& holder, StkRegFile& regs)
  : lat(countLabels(holder)),
    genv(new Dict),
    stk_regf(regs),
    holder(holder) {

    stk_top = stk.data;
    stk_base = stk.data;

    ci_top = cis.data;

    loadLabels(this);
    loadBaseLib(this);
    loadMainFunction(this);

    // Call main
    impl::__call(this, main.u.clsr);
}

State::~State() {
    delete genv;
}

} // namespace xvm
