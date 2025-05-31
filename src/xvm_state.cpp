// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_state.h"
#include "xvm_api_impl.h"
#include "xvm_lib_base.h"

namespace xvm {

using enum Opcode;

static size_t countLabels(BytecodeHolder& holder) {
    size_t lcount = 0;

    for (const auto* ptr = holder.getCode(); ptr < holder.getCode() + holder.getCodeSize(); ptr++) {
        if (ptr->op == LBL)
            lcount++;
    }

    return lcount;
}

static void loadLabels(const State* state) {
    size_t lindex = 0;

    const BytecodeHolder& holder = state->bc_holder;
    const Instruction*    pcbase = state->bc_holder.getCode();
    for (const auto* ptr = holder.getCode(); ptr < holder.getCode() + holder.getCodeSize(); ptr++) {
        if (ptr->op == LBL) {
            size_t off = ptr - holder.getCode();
            state->lat.data[off] = ptr;
        }
    }
}

static void loadMainFunction(State* state) {
    Function fun;
    fun.id = "main";
    fun.line = 0;
    fun.code = state->bc_holder.getCode();
    fun.size = state->bc_holder.getCodeSize();

    Callable c;
    c.type = CallableKind::Function;
    c.u = {.fn = fun};
    c.arity = 1;

    Closure* cl = new Closure(std::move(c));
    state->main = Value(cl);
}

State::State(ConstantHolder& k_holder, BytecodeHolder& bc_holder, StkRegFile& regs)
  : lat(countLabels(bc_holder)),
    genv(new Dict),
    stk_regf(regs),
    k_holder(k_holder),
    bc_holder(bc_holder) {

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
