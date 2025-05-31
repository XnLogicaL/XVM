// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file tdict.h
 * @brief Declares the Dict structure used for key-value mapping in the xvm VM.
 *
 * The Dict structure implements a hash table mapping string keys to `Value`s.
 * It is used to represent dictionaries and object-like structures in the xvm language.
 */

#ifndef XVM_TDICT_H
#define XVM_TDICT_H

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
 * @brief Default starting capacity for all dictionaries.
 */
inline constexpr size_t kDictCapacity = 64;

/**
 * @struct Dict
 * @brief A dynamically allocated hash table mapping `const char*` keys to `Value` objects.
 *
 * This dictionary implementation is based on open addressing (linear probing).
 * Keys are raw C strings assumed to be interned or otherwise stable.
 */
struct Dict {
    /**
     * @struct HNode
     * @brief A single key-value entry within the dictionary hash table.
     */
    struct HNode {
        const char* key;   ///< Null-terminated string key (not owned).
        Value       value; ///< Corresponding value.
    };

    HNode* data = NULL;              ///< Pointer to the hash table buffer.
    size_t capacity = kDictCapacity; ///< Total capacity of the table.
    CSize  csize = {};               ///< Tracks logical size and handles resizing.

    XVM_IMPLCOPY(Dict); ///< Enables copy constructor and assignment.
    XVM_IMPLMOVE(Dict); ///< Enables move constructor and assignment.

    Dict();
    ~Dict();
};

} // namespace xvm

/** @} */

#endif
