//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/heap/unsafe_free_list_heap.hpp>

namespace immer {
namespace detail {

template <typename Heap>
struct thread_local_free_list_storage
{
    struct head_t
    {
        free_list_node* data;
        std::size_t count;

        ~head_t() { Heap::clear(); }
    };

    static head_t& head()
    {
        thread_local static head_t head_{nullptr, 0};
        return head_;
    }
};

} // namespace detail

/*!
 * Adaptor that does not release the memory to the parent heap but
 * instead it keeps the memory in a `thread_local` global free
 * list. Must be preceded by a `with_data<free_list_node, ...>` heap
 * adaptor.  When the current thread finishes, the memory is returned
 * to the parent heap.
 *
 * @tparam Size  Maximum size of the objects to be allocated.
 * @tparam Limit Maximum number of elements to keep in the free list.
 * @tparam Base  Type of the parent heap.
 */
template <std::size_t Size, std::size_t Limit, typename Base>
struct thread_local_free_list_heap : detail::unsafe_free_list_heap_impl<
    detail::thread_local_free_list_storage,
    Size,
    Limit,
    Base>
{};

} // namespace immer
