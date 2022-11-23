//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/heap/free_list_node.hpp>
#include <immer/heap/with_data.hpp>

#include <atomic>
#include <cassert>
#include <cstddef>

namespace immer {

/*!
 * Adaptor that does not release the memory to the parent heap but
 * instead it keeps the memory in a thread-safe global free list. Must
 * be preceded by a `with_data<free_list_node, ...>` heap adaptor.
 *
 * @tparam Size Maximum size of the objects to be allocated.
 * @tparam Base Type of the parent heap.
 */
template <std::size_t Size, std::size_t Limit, typename Base>
struct free_list_heap : Base
{
    using base_t = Base;

    template <typename... Tags>
    static void* allocate(std::size_t size, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        free_list_node* n;
        do {
            n = head().data;
            if (!n) {
                auto p = base_t::allocate(Size + sizeof(free_list_node));
                return static_cast<free_list_node*>(p);
            }
        } while (!head().data.compare_exchange_weak(n, n->next));
        head().count.fetch_sub(1u, std::memory_order_relaxed);
        return n;
    }

    template <typename... Tags>
    static void deallocate(std::size_t size, void* data, Tags...)
    {
        assert(size <= sizeof(free_list_node) + Size);
        assert(size >= sizeof(free_list_node));

        // we use relaxed, because we are fine with temporarily having
        // a few more/less buffers in free list
        if (head().count.load(std::memory_order_relaxed) >= Limit) {
            base_t::deallocate(Size + sizeof(free_list_node), data);
        } else {
            auto n = static_cast<free_list_node*>(data);
            do {
                n->next = head().data;
            } while (!head().data.compare_exchange_weak(n->next, n));
            head().count.fetch_add(1u, std::memory_order_relaxed);
        }
    }

private:
    struct head_t
    {
        std::atomic<free_list_node*> data;
        std::atomic<std::size_t> count;
    };

    static head_t& head()
    {
        static head_t head_{{nullptr}, {0}};
        return head_;
    }
};

} // namespace immer
