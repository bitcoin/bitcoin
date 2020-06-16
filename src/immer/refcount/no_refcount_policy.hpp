//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace immer {

struct disowned {};

struct no_spinlock
{
    bool try_lock() { return true; }
    void lock() {}
    void unlock() {}

    struct scoped_lock
    {
        scoped_lock(no_spinlock&) {}
    };
};

/*!
 * Disables reference counting, to be used with an alternative garbage
 * collection strategy like a `gc_heap`.
 */
struct no_refcount_policy
{
    using spinlock_type = no_spinlock;

    no_refcount_policy() {};
    no_refcount_policy(disowned) {}

    void inc() {}
    bool dec() { return false; }
    void dec_unsafe() {}
    bool unique() { return false; }
};

} // namespace immer
