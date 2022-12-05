//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <nonius.h++>

#include <immer/heap/gc_heap.hpp>
#include <immer/memory_policy.hpp>

namespace {

NONIUS_PARAM(N, std::size_t{1000})

struct gc_disable
{
    gc_disable()
    {
#if IMMER_BENCHMARK_DISABLE_GC
        GC_disable();
#else
        GC_gcollect();
#endif
    }
    ~gc_disable()
    {
#if IMMER_BENCHMARK_DISABLE_GC
        GC_enable();
        GC_gcollect();
#endif
    }
    gc_disable(const gc_disable&) = delete;
    gc_disable(gc_disable&&)      = delete;
};

template <typename Meter, typename Fn>
void measure(Meter& m, Fn&& fn)
{
    gc_disable guard;
    return m.measure(std::forward<Fn>(fn));
}

using def_memory   = immer::default_memory_policy;
using gc_memory    = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy>;
using gcf_memory   = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                        immer::no_refcount_policy,
                                        immer::default_lock_policy,
                                        immer::gc_transience_policy,
                                        false>;
using basic_memory = immer::memory_policy<immer::heap_policy<immer::cpp_heap>,
                                          immer::refcount_policy,
                                          immer::default_lock_policy>;
using safe_memory =
    immer::memory_policy<immer::free_list_heap_policy<immer::cpp_heap>,
                         immer::refcount_policy,
                         immer::default_lock_policy>;
using unsafe_memory =
    immer::memory_policy<immer::unsafe_free_list_heap_policy<immer::cpp_heap>,
                         immer::unsafe_refcount_policy,
                         immer::default_lock_policy>;

} // anonymous namespace
