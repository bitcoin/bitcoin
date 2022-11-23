//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/set.hpp>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

template <typename T,
          typename Hash = std::hash<T>,
          typename Eq   = std::equal_to<T>>
using test_set_t = immer::set<T, Hash, Eq, gc_memory, 3u>;

#define SET_T test_set_t
#include "generic.ipp"
