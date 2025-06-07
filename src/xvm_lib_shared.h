// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_SHARED_H
#define XVM_SHARED_H

#include "xvm_common.h"
#include "xvm_state.h"

namespace xvm {

Callable makeNativeCallable( NativeFn ptr, size_t arity );
void declareCoreFunction( State* state, const char* id, NativeFn ptr, size_t arity );

} // namespace xvm

#endif
