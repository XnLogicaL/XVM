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
  fun.code = state->bcHolder.data();
  fun.size = state->bcHolder.size();

  Callable c;
  c.type = CallableKind::Function;
  c.u = { .fn = fun };
  c.arity = 1;

  Closure* cl = new Closure( std::move( c ) );
  state->main = Value( cl );
}

State::State(
  const std::vector<Value>& kHolder,
  const std::vector<Instruction>& bcHolder,
  const std::vector<InstructionData>& bcInfoHolder
)
  : globalEnv( new Dict ),
    kHolder( kHolder ),
    bcHolder( bcHolder ),
    bcInfoHolder( bcInfoHolder ) {

  stackTop = stack.data;
  stackBase = stack.data;

  callInfoTop = callInfoStack.data;

  loadBaseLib( this );
  loadMainFunction( this );

  // Call main
  impl::__call( this, main.u.clsr );
}

State::~State() {
  delete globalEnv;
}

} // namespace xvm
