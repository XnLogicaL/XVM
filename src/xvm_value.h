// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file object.h
 * @brief Declares the core `Value` type, a tagged union for the runtime.
 *
 * This is a polymorphic container for all dynamically typed runtime values.
 * It efficiently stores and handles different value types including numbers,
 * booleans, strings, arrays, dictionaries, and closures.
 */
#ifndef XVM_OBJECT_H
#define XVM_OBJECT_H

#include "xvm_common.h"

// MSVC is annoying with uninitialized members
#if XVMC == CMSVC
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

#define XVM_NIL Value()

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

// Forward declarations for pointer types stored in Value
struct State;
struct String;
struct Array;
struct Dict;
struct Closure;

enum class ValueKind : uint8_t {
    Nil,      ///< Null or "empty" value.
    Int,      ///< Integer value.
    Float,    ///< Floating-point value.
    Bool,     ///< Boolean value.
    String,   ///< Pointer to `String`.
    Function, ///< Pointer to `Closure`.
    Array,    ///< Pointer to `Array`.
    Dict      ///< Pointer to `Dict`.
};

/**
 * @struct Value
 * @brief Polymorphic tagged union representing any runtime value in xvm.
 *
 * This type is used throughout the xvm VM to hold and manipulate values
 * of different types dynamically at runtime.
 */
struct alignas(8) Value {
    /**
     * @enum Tag
     * @brief Discriminates the active member of the Value union.
     */
    ValueKind type;

    /**
     * @union Un
     * @brief Holds the actual value for the current tag.
     */
    union Un {
        int      i;    ///< Integer.
        float    f;    ///< Float.
        bool     b;    ///< Boolean.
        String*  str;  ///< Heap string pointer.
        Array*   arr;  ///< Heap array pointer.
        Dict*    dict; ///< Heap dictionary pointer.
        Closure* clsr; ///< Function closure pointer.
    } u;

    XVM_NOCOPY(Value);
    XVM_IMPLMOVE(Value);

    ~Value();

    // Constructors
    explicit Value();             ///< Default constructor (Nil).
    explicit Value(bool b);       ///< Constructs a Bool.
    explicit Value(int x);        ///< Constructs an Int.
    explicit Value(float x);      ///< Constructs a Float.
    explicit Value(String* ptr);  ///< Constructs a String.
    explicit Value(Array* ptr);   ///< Constructs an Array.
    explicit Value(Dict* ptr);    ///< Constructs a Dict.
    explicit Value(Closure* ptr); ///< Constructs a Closure.
};

} // namespace xvm

/** @} */

#if XVMC == CMSVC
#pragma warning(pop)
#endif

#endif // XVM_OBJECT_H
