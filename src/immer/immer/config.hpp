//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(nodiscard)
#define IMMER_NODISCARD [[nodiscard]]
#endif
#else
#if _MSVC_LANG >= 201703L
#define IMMER_NODISCARD [[nodiscard]]
#endif
#endif

#ifdef __has_feature
#if !__has_feature(cxx_exceptions)
#define IMMER_NO_EXCEPTIONS
#endif
#endif

#ifdef IMMER_NO_EXCEPTIONS
#define IMMER_TRY if (true)
#define IMMER_CATCH(expr) else
#define IMMER_THROW(expr)                                                      \
    do {                                                                       \
        assert(!#expr);                                                        \
        std::terminate();                                                      \
    } while (false)
#define IMMER_RETHROW
#else
#define IMMER_TRY try
#define IMMER_CATCH(expr) catch (expr)
#define IMMER_THROW(expr) throw expr
#define IMMER_RETHROW throw
#endif

#ifndef IMMER_NODISCARD
#define IMMER_NODISCARD
#endif

#ifndef IMMER_TAGGED_NODE
#ifdef NDEBUG
#define IMMER_TAGGED_NODE 0
#else
#define IMMER_TAGGED_NODE 1
#endif
#endif

#if IMMER_TAGGED_NODE
#define IMMER_ASSERT_TAGGED(assertion) assert(assertion)
#else
#define IMMER_ASSERT_TAGGED(assertion)
#endif

#ifndef IMMER_DEBUG_TRACES
#define IMMER_DEBUG_TRACES 0
#endif

#ifndef IMMER_DEBUG_PRINT
#define IMMER_DEBUG_PRINT 0
#endif

#ifndef IMMER_DEBUG_DEEP_CHECK
#define IMMER_DEBUG_DEEP_CHECK 0
#endif

#if IMMER_DEBUG_TRACES || IMMER_DEBUG_PRINT
#include <iostream>
#include <prettyprint.hpp>
#endif

#if IMMER_DEBUG_TRACES
#define IMMER_TRACE(...) std::cout << __VA_ARGS__ << std::endl
#else
#define IMMER_TRACE(...)
#endif
#define IMMER_TRACE_F(...)                                                     \
    IMMER_TRACE(__FILE__ << ":" << __LINE__ << ": " << __VA_ARGS__)
#define IMMER_TRACE_E(expr) IMMER_TRACE("    " << #expr << " = " << (expr))

#if defined(_MSC_VER)
#define IMMER_UNREACHABLE __assume(false)
#define IMMER_LIKELY(cond) cond
#define IMMER_UNLIKELY(cond) cond
#define IMMER_FORCEINLINE __forceinline
#define IMMER_PREFETCH(p)
#else
#define IMMER_UNREACHABLE __builtin_unreachable()
#define IMMER_LIKELY(cond) __builtin_expect(!!(cond), 1)
#define IMMER_UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#define IMMER_FORCEINLINE inline __attribute__((always_inline))
#define IMMER_PREFETCH(p)
// #define IMMER_PREFETCH(p)    __builtin_prefetch(p)
#endif

#define IMMER_DESCENT_DEEP 0

#ifndef IMMER_ENABLE_DEBUG_SIZE_HEAP
#ifdef NDEBUG
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 0
#else
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 1
#endif
#endif

namespace immer {

const auto default_bits           = 5;
const auto default_free_list_size = 1 << 10;

} // namespace immer
