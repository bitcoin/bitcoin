//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

namespace immer {

struct no_lock_policy
{
    bool try_lock() { return true; }
    void lock() {}
    void unlock() {}

    struct scoped_lock
    {
        scoped_lock(no_lock_policy&) {}
    };
};

} // namespace immer
