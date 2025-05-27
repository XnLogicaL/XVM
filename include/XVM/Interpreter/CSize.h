// This file is a part of the xvm Programming Language project
// Copyright (C) 2024-2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file csize.h
 * @brief Size caching utility
 */
#ifndef XVM_CSIZE_H
#define XVM_CSIZE_H

#include <Common.h>

/**
 * @namespace xvm
 */
namespace xvm {

/**
 * @struct CSize
 * @ingroup xvm_namespace
 * @brief Small utility size caching structure
 */
struct CSize {
    mutable bool valid = false;
    mutable bool cache = 0;
};

} // namespace xvm

#endif