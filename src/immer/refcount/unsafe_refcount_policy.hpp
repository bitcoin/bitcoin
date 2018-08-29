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
#include <utility>

namespace immer {

/*!
 * A reference counting policy implemented using a raw `int` count.
 * It is **not thread-safe**.
 */
struct unsafe_refcount_policy
{
    mutable int refcount;

    unsafe_refcount_policy() : refcount{1} {};
    unsafe_refcount_policy(disowned) : refcount{0} {}

    void inc() { ++refcount; }
    bool dec() { return --refcount == 0; }
    void dec_unsafe() { --refcount; }
    bool unique() { return refcount == 1; }
};

} // namespace immer
