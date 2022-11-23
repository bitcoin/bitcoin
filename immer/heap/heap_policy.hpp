//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/config.hpp>
#include <immer/heap/debug_size_heap.hpp>
#include <immer/heap/free_list_heap.hpp>
#include <immer/heap/split_heap.hpp>
#include <immer/heap/thread_local_free_list_heap.hpp>

#include <algorithm>
#include <cstdlib>

namespace immer {

/*!
 * Heap policy that unconditionally uses its `Heap` argument.
 */
template <typename Heap>
struct heap_policy
{
    using type = Heap;

    template <std::size_t>
    struct optimized
    {
        using type = Heap;
    };
};

template <typename Deriv, typename HeapPolicy>
struct enable_optimized_heap_policy
{
    static void* operator new(std::size_t size)
    {
        using heap_type =
            typename HeapPolicy ::template optimized<sizeof(Deriv)>::type;

        return heap_type::allocate(size);
    }

    static void operator delete(void* data, std::size_t size)
    {
        using heap_type =
            typename HeapPolicy ::template optimized<sizeof(Deriv)>::type;

        heap_type::deallocate(size, data);
    }
};

/*!
 * Heap policy that returns a heap with a free list of objects
 * of `max_size = max(Sizes...)` on top an underlying `Heap`.  Note
 * these two properties of the resulting heap:
 *
 * - Allocating an object that is bigger than `max_size` may trigger
 *   *undefined behavior*.
 *
 * - Allocating an object of size less than `max_size` still
 *   returns an object of `max_size`.
 *
 * Basically, this heap will always return objects of `max_size`.
 * When an object is freed, it does not directly invoke `std::free`,
 * but it keeps the object in a global linked list instead.  When a
 * new object is requested, it does not need to call `std::malloc` but
 * it can directly pop and return the other object from the global
 * list, a much faster operation.
 *
 * This actually creates a hierarchy with two free lists:
 *
 * - A `thread_local` free list is used first.  It does not need any
 *   kind of synchronization and is very fast.  When the thread
 *   finishes, its contents are returned to the next free list.
 *
 * - A global free list using lock-free access via atomics.
 *
 * @tparam Heap Heap to be used when the free list is empty.
 *
 * @rst
 *
 * .. tip:: For many applications that use immutable data structures
 *    significantly, this is actually the best heap policy, and it
 *    might become the default in the future.
 *
 *    Note that most our data structures internally use trees with the
 *    same big branching factors.  This means that all *vectors*,
 *    *maps*, etc. can just allocate elements from the same free-list
 *    optimized heap.  Not only does this lowers the allocation time,
 *    but also makes up for more efficient *cache utilization*.  When
 *    a new node is needed, there are high chances the allocator will
 *    return a node that was just accessed.  When batches of immutable
 *    updates are made, this can make a significant difference.
 *
 * @endrst
 */
template <typename Heap, std::size_t Limit = default_free_list_size>
struct free_list_heap_policy
{
    using type = debug_size_heap<Heap>;

    template <std::size_t Size>
    struct optimized
    {
        using type =
            split_heap<Size,
                       with_free_list_node<thread_local_free_list_heap<
                           Size,
                           Limit,
                           free_list_heap<Size, Limit, debug_size_heap<Heap>>>>,
                       debug_size_heap<Heap>>;
    };
};

/*!
 * Similar to @ref free_list_heap_policy, but it assumes no
 * multi-threading, so a single global free list with no concurrency
 * checks is used.
 */
template <typename Heap, std::size_t Limit = default_free_list_size>
struct unsafe_free_list_heap_policy
{
    using type = Heap;

    template <std::size_t Size>
    struct optimized
    {
        using type = split_heap<
            Size,
            with_free_list_node<
                unsafe_free_list_heap<Size, Limit, debug_size_heap<Heap>>>,
            debug_size_heap<Heap>>;
    };
};

} // namespace immer
