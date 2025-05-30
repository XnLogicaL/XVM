// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_COMMON_H
#define XVM_COMMON_H

// C++ std imports
#include <bitset>
#include <cassert>
#include <cctype>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#define CGCC   0 // g++ compiler
#define CCLANG 1 // clang++ compiler
#define CMSVC  2 // msvc compiler

#ifdef __GNUC__
#ifdef __clang__
#define XVMC CCLANG
#else
#define XVMC CGCC
#endif
#elif defined(_MSC_VER)
#define XVMC CMSVC
#else
#error "Unknown compiler"
#endif

#if XVMC == CGCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultistatement-macros"
#endif

// Check if libstacktrace is available.
#if __has_include(<stacktrace>) && __cplusplus >= 202302L
#include <stacktrace>
#define XVM_HASSTACKTRACE 1
#else
#define XVM_HASSTACKTRACE 0
#endif

// Version information. Should match with git commit version.
#define XVM_VERSION "1.0.1"

#if XVMC == CMSVC
#define XVM_RESTRICT      __restrict
#define XVM_NOMANGLE      extern "C"
#define XVM_NODISCARD     [[nodiscard]]
#define XVM_NORETURN      __declspec(noreturn)
#define XVM_NOINLINE      __declspec(noinline)
#define XVM_FORCEINLINE   __forceinline
#define XVM_OPTIMIZE      __forceinline // MSVC doesn't have 'hot' attribute
#define XVM_UNREACHABLE() __assume(0)
#define XVM_FUNCSIG       __FUNCSIG__
#define XVM_LIKELY(x)     (x) // No branch prediction hints in MSVC
#define XVM_UNLIKELY(x)   (x)
#else // GCC / Clang
#define XVM_RESTRICT      __restrict__
#define XVM_NOMANGLE      extern "C"
#define XVM_NODISCARD     [[nodiscard]]
#define XVM_NORETURN      __attribute__((noreturn))
#define XVM_NOINLINE      __attribute__((noinline))
#define XVM_FORCEINLINE   inline __attribute__((always_inline))
#define XVM_OPTIMIZE      inline __attribute__((always_inline, hot))
#define XVM_UNREACHABLE() __builtin_unreachable()
#define XVM_FUNCSIG       __PRETTY_FUNCTION__
#define XVM_LIKELY(a)     (__builtin_expect(!!(a), 1))
#define XVM_UNLIKELY(a)   (__builtin_expect(!!(a), 0))
#endif

#define XVM_GLOBAL inline

/**
 * Makes the target class or struct uncopyable in terms of copy semantics.
 * Must be used inside class or struct clause.
 */
#define XVM_NOCOPY(target)                                                                         \
    target& operator=(const target&) = delete;                                                     \
    target(const target&) = delete;

/**
 * Makes the target class implement custom copy semantics.
 * Must be used inside class or struct clause.
 */
#define XVM_IMPLCOPY(target)                                                                       \
    target& operator=(const target&);                                                              \
    target(const target&);

/**
 * Makes the target class or struct unmovable in terms of move semantics.
 * Must be used inside class or struct clause.
 */
#define XVM_NOMOVE(target)                                                                         \
    target& operator=(target&&) = delete;                                                          \
    target(target&&) = delete;

/**
 * Makes the target class implement custom move semantics.
 * Must be used inside class or struct clause.
 */
#define XVM_IMPLMOVE(target)                                                                       \
    target& operator=(target&&);                                                                   \
    target(target&&);

#if XVM_HASSTACKTRACE == 1
#define XVM_STACKTRACE std::stacktrace::current()
#else
#define XVM_STACKTRACE ""
#endif

/**
 * Custom assertion macro that contains debug information like condition, file, line, message and
 * stacktrace on some platforms.
 * Uses stderr to buffer the output and then calls std::abort().
 */
#define XVM_ASSERT(condition, message)                                                             \
    if (!(condition)) {                                                                            \
        std::cerr << "XVM_ASSERT(): assertion '" << #condition << "' failed.\n"                    \
                  << "location: " << __FILE__ << ":" << __LINE__ << "\nmessage: " << message       \
                  << "\n";                                                                         \
        if (XVM_HASSTACKTRACE) {                                                                   \
            std::cerr << "cstk:\n" << XVM_STACKTRACE << '\n';                                      \
        }                                                                                          \
        std::abort();                                                                              \
    }

#if XVMC == CGCC
#pragma GCC diagnostic pop
#endif

#endif
