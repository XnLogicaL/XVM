// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_lib_base.h"

namespace xvm {

static Value core_print( State* state ) {
  Value* arg0 = impl::__getArgument( state, 0 );
  std::cout << arg0->to_cxx_string() << "\n";
  return XVM_NIL;
}

static Value core_error( State* state ) {
  Value* arg0 = impl::__getArgument( state, 0 );
  impl::__ethrow( state, arg0->to_cxx_string() );
  return XVM_NIL;
}

void loadBaseLib( State* state ) {
  declareCoreFunction( state, "print", core_print, 1 );
  declareCoreFunction( state, "error", core_error, 1 );
}

} // namespace xvm
