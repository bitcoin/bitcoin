//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

// include:example/start
#include <immer/heap/gc_heap.hpp>
#include <immer/heap/heap_policy.hpp>
#include <immer/memory_policy.hpp>
#include <immer/refcount/no_refcount_policy.hpp>
#include <immer/vector.hpp>

#include <iostream>

// declare a memory policy for using a tracing garbage collector
using gc_policy = immer::memory_policy<immer::heap_policy<immer::gc_heap>,
                                       immer::no_refcount_policy,
                                       immer::default_lock_policy,
                                       immer::gc_transience_policy,
                                       false>;

// alias the vector type so we are not concerned about memory policies
// in the places where we actually use it
template <typename T>
using my_vector = immer::vector<T, gc_policy>;

int main()
{
    auto v =
        my_vector<const char*>().push_back("hello, ").push_back("world!\n");

    for (auto s : v)
        std::cout << s;
}
// include:example/end
