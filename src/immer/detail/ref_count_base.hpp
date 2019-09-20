//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <atomic>

namespace immer {
namespace detail {

template <typename Deriv>
struct ref_count_base
{
    mutable std::atomic<int> ref_count { 0 };

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        x->ref_count.fetch_add(1, std::memory_order_relaxed);
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (x->ref_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete x;
        }
    }
};

} /* namespace detail */
} /* namespace immer */
