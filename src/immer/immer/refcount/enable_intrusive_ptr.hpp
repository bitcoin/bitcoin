//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <immer/refcount/no_refcount_policy.hpp>

namespace immer {

template <typename Deriv, typename RefcountPolicy>
class enable_intrusive_ptr
{
    mutable RefcountPolicy refcount_data_;

public:
    enable_intrusive_ptr()
        : refcount_data_{disowned{}}
    {}

    friend void intrusive_ptr_add_ref(const Deriv* x)
    {
        x->refcount_data_.inc();
    }

    friend void intrusive_ptr_release(const Deriv* x)
    {
        if (x->refcount_data_.dec())
            delete x;
    }
};

} // namespace immer
