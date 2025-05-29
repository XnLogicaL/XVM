// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>

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
    size_t       lindex = 0;
    Instruction* pcbase = state->holder.insns.data();

    for (size_t counter = 0; const Instruction& insn : state->holder.insns) {
        if (insn.op == LBL)
            state->laddress_table.data[lindex++] = pcbase + counter;
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


static Callable makeNativeCallable(NativeFn ptr, size_t arity) {
    Callable c;
    c.type = CallableKind::Native;
    c.u.ntv = ptr;
    c.arity = arity;
    return c;
}

static void declareCoreFunction(State* state, const char* id, NativeFn ptr, size_t arity) {
    Callable c = makeNativeCallable(ptr, arity);
    Closure* cl = new Closure(std::move(c));

    impl::__setDictField(state->globals, id, Value(cl));
}

static void declareCoreLib(State* state) {
    static NativeFn core_print = [](State* state) -> Value {
        Value* arg0 = impl::__getRegister(state, state->args);
        std::cout << arg0->to_cxx_string() << "\n";
        return XVM_NIL;
    };

    static NativeFn core_error = [](State* state) -> Value {
        Value* arg0 = impl::__getRegister(state, state->args);
        impl::__setError(state, arg0->to_cxx_string());
        return XVM_NIL;
    };

    declareCoreFunction(state, "print", core_print, 1);
    declareCoreFunction(state, "error", core_error, 1);
}

State::State(BytecodeHolder& holder, StkRegFile& regs)
  : laddress_table(countLabels(holder)),
    globals(new Dict),
    stk_regf(regs),
    holder(holder),
    str_alloc(1024 * 256) {

    loadLabels(this);
    declareCoreLib(this);
    loadMainFunction(this);

    // Call main
    impl::__call(this, main.u.clsr);
}

State::~State() {
    delete globals;
}

} // namespace xvm
