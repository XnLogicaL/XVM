// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_closure.h"
#include "xvm_api_impl.h"

namespace xvm {

Closure::Closure( Callable&& callable, size_t upvCount )
  : callee( callable ),
    upvs( upvCount ) {}

} // namespace xvm
