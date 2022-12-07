//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#include <immer/heap/cpp_heap.hpp>
#include <immer/heap/free_list_heap.hpp>
#include <immer/heap/gc_heap.hpp>
#include <immer/heap/malloc_heap.hpp>
#include <immer/heap/thread_local_free_list_heap.hpp>

#include <catch.hpp>
#include <numeric>

void do_stuff_to(void* buf, std::size_t size)
{
    auto ptr = static_cast<unsigned char*>(buf);
    CHECK(buf != 0);
    std::iota(ptr, ptr + size, 42);
}

TEST_CASE("malloc")
{
    using heap = immer::malloc_heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(42, p);
    }
}

TEST_CASE("gc")
{
    using heap = immer::gc_heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(42, p);
    }
}

TEST_CASE("cpp")
{
    using heap = immer::cpp_heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(42, p);
    }
}

template <typename Heap>
void test_free_list_heap()
{
    using heap = Heap;

    SECTION("basic")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(42, p);
    }

    SECTION("reuse")
    {
        auto p = heap::allocate(42u);
        do_stuff_to(p, 42u);
        heap::deallocate(42, p);

        auto u = heap::allocate(12u);
        do_stuff_to(u, 12u);
        heap::deallocate(12, u);
        CHECK(u == p);
    }
}

TEST_CASE("free list")
{
    test_free_list_heap<immer::free_list_heap<42u, 2, immer::malloc_heap>>();
}

TEST_CASE("thread local free list")
{
    test_free_list_heap<
        immer::thread_local_free_list_heap<42u, 2, immer::malloc_heap>>();
}

TEST_CASE("unsafe free_list")
{
    test_free_list_heap<
        immer::unsafe_free_list_heap<42u, 2, immer::malloc_heap>>();
}
