#ifndef BOOST_NUMERIC_SAFE_INTEGER_RANGE_HPP
#define BOOST_NUMERIC_SAFE_INTEGER_RANGE_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint> // intmax_t, uintmax_t

#include "utility.hpp"
#include "safe_integer.hpp"
#include "native.hpp"
#include "exception_policies.hpp"

/////////////////////////////////////////////////////////////////
// higher level types implemented in terms of safe_base

namespace boost {
namespace safe_numerics {

/////////////////////////////////////////////////////////////////
// safe_signed_range

template <
    std::intmax_t Min,
    std::intmax_t Max,
    class P = native,
    class E = default_exception_policy
>
using safe_signed_range = safe_base<
    typename utility::signed_stored_type<Min, Max>,
    static_cast<typename utility::signed_stored_type<Min, Max> >(Min),
    static_cast<typename utility::signed_stored_type<Min, Max> >(Max),
    P,
    E
>;

/////////////////////////////////////////////////////////////////
// safe_unsigned_range

template <
    std::uintmax_t Min,
    std::uintmax_t Max,
    class P = native,
    class E = default_exception_policy
>
using safe_unsigned_range = safe_base<
    typename utility::unsigned_stored_type<Min, Max>,
    static_cast<typename utility::unsigned_stored_type<Min, Max> >(Min),
    static_cast<typename utility::unsigned_stored_type<Min, Max> >(Max),
    P,
    E
>;

} // safe_numerics
} // boost

#endif // BOOST_NUMERIC_SAFE_RANGE_HPP
