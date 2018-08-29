//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

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
#define IMMER_TRACE_F(...)                                              \
    IMMER_TRACE(__FILE__ << ":" << __LINE__ << ": " << __VA_ARGS__)
#define IMMER_TRACE_E(expr)                             \
    IMMER_TRACE("    " << #expr << " = " << (expr))

#define IMMER_UNREACHABLE    __builtin_unreachable()
#define IMMER_LIKELY(cond)   __builtin_expect(!!(cond), 1)
#define IMMER_UNLIKELY(cond) __builtin_expect(!!(cond), 0)
// #define IMMER_PREFETCH(p)    __builtin_prefetch(p)
#define IMMER_PREFETCH(p)
#define IMMER_FORCEINLINE    inline __attribute__ ((always_inline))

#define IMMER_DESCENT_DEEP 0

#ifdef NDEBUG
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 0
#else
#define IMMER_ENABLE_DEBUG_SIZE_HEAP 1
#endif

namespace immer {

const auto default_bits = 5;
const auto default_free_list_size = 1 << 10;

} // namespace immer
