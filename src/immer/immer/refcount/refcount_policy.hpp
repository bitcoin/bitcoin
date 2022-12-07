//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/refcount/no_refcount_policy.hpp>

#include <atomic>
#include <cassert>
#include <utility>

namespace immer {

/*!
 * A reference counting policy implemented using an *atomic* `int`
 * count.  It is **thread-safe**.
 */
struct refcount_policy
{
    mutable std::atomic<int> refcount;

    refcount_policy()
        : refcount{1} {};
    refcount_policy(disowned)
        : refcount{0}
    {}

    void inc() { refcount.fetch_add(1, std::memory_order_relaxed); }

    bool dec() { return 1 == refcount.fetch_sub(1, std::memory_order_acq_rel); }

    void dec_unsafe()
    {
        assert(refcount.load() > 1);
        refcount.fetch_sub(1, std::memory_order_relaxed);
    }

    bool unique() { return refcount == 1; }
};

} // namespace immer
