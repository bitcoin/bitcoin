//
// immer: immutable data structures for C++
// Copyright (C) 2016, 2017, 2018 Juan Pedro Bolivar Puente
//
// This software is distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://boost.org/LICENSE_1_0.txt
//

#pragma once

#include <scm/val.hpp>
#include <iostream>

namespace scm {

struct list : detail::wrapper
{
    using base_t = detail::wrapper;
    using base_t::base_t;

    using iterator = list;
    using value_type = val;

    list() : base_t{SCM_EOL} {};
    list end() const { return {}; }
    list begin() const { return *this; }

    explicit operator bool() { return handle_ != SCM_EOL; }

    val operator* () const { return val{scm_car(handle_)}; }

    list& operator++ ()
    {
        handle_ = scm_cdr(handle_);
        return *this;
    }

    list operator++ (int)
    {
        auto result = *this;
        result.handle_ = scm_cdr(handle_);
        return result;
    }
};

struct args : list
{
    using list::list;
};

} // namespace scm

SCM_DECLARE_WRAPPER_TYPE(scm::list);
SCM_DECLARE_WRAPPER_TYPE(scm::args);
