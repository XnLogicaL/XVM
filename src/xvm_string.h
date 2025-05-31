// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file tstring.h
 * @brief Defines the `String` type used within the virtual machine and runtime.
 *
 * This is a constant-sized, heap-allocated, hash-cached string structure
 * designed for performance in dictionary lookups and language runtime .
 */
#ifndef XVM_STRING_H
#define XVM_STRING_H

#include "xvm_common.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

char* strdup(const char* str);
char* strdup(const std::string& str);

uint32_t strhash(const char* str);
uint32_t strhash(const std::string& str);

std::string stresc(const std::string& str);

/**
 * @struct String
 * @brief Constant-sized owning string type used in the xvm runtime.
 *
 * This structure owns its character data, tracks its size, and caches a hash
 * value to accelerate dictionary operations and comparisons.
 */
struct String {
    char*    data = NULL; ///< Heap-allocated UTF-8 character data.
    size_t   size = 0;    ///< Number of bytes in the string (not null-terminated).
    uint32_t hash = 0;    ///< Cached hash for fast comparisons and dict lookup.

    XVM_IMPLCOPY(String);
    XVM_IMPLMOVE(String);

    /**
     * @brief Constructs a new `String` from a null-terminated C-string.
     * @param str The input C-string to copy.
     */
    String(const char* str);
    ~String();

    /**
     * @brief Gets a single-character string at the specified position.
     * @param position Index of the character to retrieve.
     * @return A new `String` containing just that character.
     */
    String get(size_t position);

    /**
     * @brief Replaces the character at the given index with the first character of another string.
     * @param position Index of the character to replace.
     * @param value A string whose first character will be used as the replacement.
     */
    void set(size_t position, const String& value);
};

} // namespace xvm

/** @} */

#endif
