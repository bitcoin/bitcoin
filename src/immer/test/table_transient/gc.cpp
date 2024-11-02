//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/table.hpp>
#include <immer/table_transient.hpp>

struct setup_t
{
    using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                           immer::no_refcount_policy,
                                           immer::default_lock_policy,
                                           immer::gc_transience_policy,
                                           false>;

    template <typename T>
    using table_transient = immer::table_transient<
        T,
        immer::table_key_fn,
        std::hash<immer::table_key_t<immer::table_key_fn, T>>,
        std::equal_to<immer::table_key_t<immer::table_key_fn, T>>,
        gc_memory,
        immer::default_bits>;

    template <typename T>
    using table =
        immer::table<T,
                     immer::table_key_fn,
                     std::hash<immer::table_key_t<immer::table_key_fn, T>>,
                     std::equal_to<immer::table_key_t<immer::table_key_fn, T>>,
                     gc_memory,
                     immer::default_bits>;
};
#define SETUP_T setup_t

#include "generic.ipp"
