//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/heap/tags.hpp>

#if IMMER_HAS_LIBGC
#include <gc/gc.h>
#else
#error "Using garbage collection requires libgc"
#endif

#include <cassert>
#include <cstdlib>
#include <exception>
#include <memory>

namespace immer {

#ifdef IMMER_GC_REQUIRE_INIT
#define IMMER_GC_REQUIRE_INIT_ IMMER_GC_REQUIRE_INIT
#elifdef __APPLE__
#define IMMER_GC_REQUIRE_INIT_ 1
#else
#define IMMER_GC_REQUIRE_INIT_ 0
#endif

#if IMMER_GC_REQUIRE_INIT_

namespace detail {

template <int Dummy = 0>
struct gc_initializer
{
    gc_initializer() { GC_INIT(); }
    static gc_initializer init;
};

template <int D>
gc_initializer<D> gc_initializer<D>::init{};

inline void gc_initializer_guard()
{
    static gc_initializer<> init_ = gc_initializer<>::init;
    (void) init_;
}

} // namespace detail

#define IMMER_GC_INIT_GUARD_ ::immer::detail::gc_initializer_guard()

#else

#define IMMER_GC_INIT_GUARD_

#endif // IMMER_GC_REQUIRE_INIT_

/*!
 * Heap that uses a tracing garbage collector.
 *
 * @rst
 *
 * This heap uses the `Boehm's conservative garbage collector`_ under
 * the hood.  This is a tracing garbage collector that automatically
 * reclaims unused memory.  Thus, it is not needed to call
 * ``deallocate()`` in order to release memory.
 *
 * .. admonition:: Dependencies
 *    :class: tip
 *
 *    In order to use this header file, you need to make sure that
 *    Boehm's ``libgc`` is your include path and link to its binary
 *    library.
 *
 * .. caution:: Memory that is allocated with the standard ``malloc``
 *    and ``free`` is not visible to ``libgc`` when it is looking for
 *    references.  This means that if, let's say, you store a
 *    :cpp:class:`immer::vector` using a ``gc_heap`` inside a
 *    ``std::vector`` that uses a standard allocator, the memory of
 *    the former might be released automatically at unexpected times
 *    causing crashes.
 *
 * .. caution:: When using a ``gc_heap`` in combination with immutable
 *    containers, the destructors of the contained objects will never
 *    be called.  It is ok to store containers inside containers as
 *    long as all of them use a ``gc_heap`` consistently, but storing
 *    other kinds of objects with relevant destructors
 *    (e.g. containers with reference counting or other kinds of
 *    *resource handles*) might cause memory leaks and other problems.
 *
 * .. _boehm's conservative garbage collector: https://github.com/ivmai/bdwgc
 *
 * @endrst
 */
class gc_heap
{
public:
    static void* allocate(std::size_t n)
    {
        IMMER_GC_INIT_GUARD_;
        auto p = GC_malloc(n);
        if (IMMER_UNLIKELY(!p))
            IMMER_THROW(std::bad_alloc{});
        return p;
    }

    static void* allocate(std::size_t n, norefs_tag)
    {
        IMMER_GC_INIT_GUARD_;
        auto p = GC_malloc_atomic(n);
        if (IMMER_UNLIKELY(!p))
            IMMER_THROW(std::bad_alloc{});
        return p;
    }

    static void deallocate(std::size_t, void* data) { GC_free(data); }

    static void deallocate(std::size_t, void* data, norefs_tag)
    {
        GC_free(data);
    }
};

} // namespace immer
