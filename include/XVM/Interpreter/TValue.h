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

#include <Common.h>

// MSVC is annoying with uninitialized members
#if XVMC == CMSVC
#pragma warning(push)
#pragma warning(disable : 26495)
#endif

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
        int i;         ///< Integer.
        float f;       ///< Float.
        bool b;        ///< Boolean.
        String* str;   ///< Heap string pointer.
        Array* arr;    ///< Heap array pointer.
        Dict* dict;    ///< Heap dictionary pointer.
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

    Value clone() const; ///< Deep copy of the value.
    void reset();        ///< Clears the value and resets to Nil.

    // clang-format off
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is(ValueKind other) const { return type == other; }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_nil() const { return is(ValueKind::Nil); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_bool() const { return is(ValueKind::Bool); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_int() const { return is(ValueKind::Int); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_float() const { return is(ValueKind::Float); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_number() const { return is_int() || is_float(); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_string() const { return is(ValueKind::String); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_array() const { return is(ValueKind::Array); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_dict() const { return is(ValueKind::Dict); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_subscriptable() const { return is_string() || is_array() || is_dict(); }
    XVM_NODISCARD XVM_FORCEINLINE constexpr bool is_function() const { return is(ValueKind::Function); }
    // clang-format on

    // Conversions
    XVM_NODISCARD Value to_integer() const; ///< Attempts to convert to Int.
    XVM_NODISCARD Value to_float() const;   ///< Attempts to convert to Float.
    XVM_NODISCARD Value to_boolean() const; ///< Converts to Bool (truthiness).
    XVM_NODISCARD Value to_string() const;  ///< Converts to a String object.

    XVM_NODISCARD std::string to_cxx_string() const;         ///< Converts to a std::string.
    XVM_NODISCARD std::string to_literal_cxx_string() const; ///< String with literals escaped.

    // Type representation
    XVM_NODISCARD Value type_string() const;           ///< Returns type name as xvm::String.
    XVM_NODISCARD std::string type_cxx_string() const; ///< Returns type name as std::string.

    /**
     * @brief Attempts to obtain a raw pointer for the value.
     *
     * Only heap-allocated types (e.g., strings, arrays, dicts, closures) will
     * return a valid pointer. Used primarily for hashing and identity checks.
     *
     * @return Pointer or NULL.
     */
    XVM_NODISCARD void* to_pointer() const;

    // Length functions
    XVM_NODISCARD Value length() const;      ///< Returns the "length" of value if possible.
    XVM_NODISCARD size_t cxx_length() const; ///< Returns native length.

    // Comparison
    XVM_NODISCARD bool compare(const Value& other) const;      ///< Shallow equality check.
    XVM_NODISCARD bool deep_compare(const Value& other) const; ///< Deep equality check.
};

} // namespace xvm

/** @} */

#if XVMC == CMSVC
#pragma warning(pop)
#endif

#endif // XVM_OBJECT_H
