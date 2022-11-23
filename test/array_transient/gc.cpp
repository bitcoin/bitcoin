//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/array.hpp>
#include <immer/array_transient.hpp>

#include <immer/heap/gc_heap.hpp>
#include <immer/refcount/no_refcount_policy.hpp>

using gc_memory = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

template <typename T>
using test_array_t = immer::array<T, gc_memory>;

template <typename T>
using test_array_transient_t = immer::array_transient<T, gc_memory>;

#define VECTOR_T test_array_t
#define VECTOR_TRANSIENT_T test_array_transient_t

#include "../vector_transient/generic.ipp"

// this comment is here because generic.ipp otherwise defines a test in the same
// line, lol!
TEST_CASE("array provides mutable data")
{
    auto arr = immer::array<int, gc_memory>(10, 0);
    CHECK(arr.size() == 10);
    auto tr = arr.transient();
    CHECK(tr.data() == arr.data());

    auto d = tr.data_mut();
    CHECK(tr.data_mut() != arr.data());
    CHECK(tr.data() == tr.data_mut());
    CHECK(arr.data() != tr.data_mut());

    arr = tr.persistent();
    CHECK(arr.data() == d);
    CHECK(arr.data() == tr.data());

    CHECK(tr.data_mut() != arr.data());
    CHECK(tr.data() == tr.data_mut());
    CHECK(arr.data() != tr.data_mut());
}
