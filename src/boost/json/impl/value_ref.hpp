//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_IMPL_VALUE_REF_HPP
#define BOOST_JSON_IMPL_VALUE_REF_HPP

#include <boost/json/value_from.hpp>

BOOST_JSON_NS_BEGIN

template<class T>
value
value_ref::
from_builtin(
    void const* p,
    storage_ptr sp) noexcept
{
    return value(
        *reinterpret_cast<
            T const*>(p),
        std::move(sp));
}

template<class T>
value
value_ref::
from_const(
    void const* p,
    storage_ptr sp)
{
    return value_from(
        *reinterpret_cast<
            T const*>(p),
        std::move(sp));
}

template<class T>
value
value_ref::
from_rvalue(
    void* p,
    storage_ptr sp)
{
    return value_from(
        std::move(
            *reinterpret_cast<T*>(p)),
        std::move(sp));
}

BOOST_JSON_NS_END

#endif
