// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file tarray.h
 * @brief Declares the Array structure used for dynamic, indexed storage in the virtual machine.
 *
 * The Array structure implements a dynamic array of `Value`s with automatic resizing
 * and index-based access. Arrays are core collection types in the xvm language runtime.
 */
#ifndef XVM_ARRAY_H
#define XVM_ARRAY_H

#include "xvm_common.h"
#include "xvm_csize.h"
#include "xvm_value.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @brief Default starting capacity for all arrays.
 */
inline constexpr size_t kArrayCapacity = 64;

/**
 * @struct Array
 * @brief A growable, dynamically sized array of `Value` elements.
 * @see CSize
 *
 * This structure wraps a heap-allocated buffer of `Value` entries and supports
 * index-based access with automatic capacity expansion. Internally, resizing is
 * delegated to the `CSize` helper, which tracks the logical size and performs bounds checks.
 */
struct Array {
    Value* data = NULL;               ///< Pointer to array data buffer.
    size_t capacity = kArrayCapacity; ///< Allocated capacity.
    CSize  csize = {};                ///< Logical size and resizing helper.

    XVM_IMPLCOPY(Array);
    XVM_IMPLMOVE(Array);

    Array();
    ~Array();
};

} // namespace xvm

/** @} */

#endif
