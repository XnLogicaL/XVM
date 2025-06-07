// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_state.h"
#include "xvm_api_impl.h"
#include "xvm_lib_base.h"

namespace xvm {

using enum Opcode;

static void loadMainFunction( State* state ) {
  Function fun;
  fun.id = "main";
  fun.line = 0;
  fun.code = state->bc_holder.data();
  fun.size = state->bc_holder.size();

  Callable c;
  c.type = CallableKind::Function;
  c.u = { .fn = fun };
  c.arity = 1;

  Closure* cl = new Closure( std::move( c ) );
  state->main = Value( cl );
}

State::State(
  const std::vector<Value>& k_holder,
  const std::vector<Instruction>& bc_holder,
  const std::vector<InstructionData>& bc_info_holder
)
  : genv( new Dict ),
    k_holder( k_holder ),
    bc_holder( bc_holder ),
    bc_info_holder( bc_info_holder ) {

  stk_top = stk.data;
  stk_base = stk.data;

  ci_top = cis.data;

  loadBaseLib( this );
  loadMainFunction( this );

  // Call main
  impl::__call( this, main.u.clsr );
}

State::~State() {
  delete genv;
}

} // namespace xvm
