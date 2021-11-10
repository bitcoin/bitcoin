#ifndef BOOST_NUMERIC_SAFE_INTEGER_HPP
#define BOOST_NUMERIC_SAFE_INTEGER_HPP

//  Copyright (c) 2012 Robert Ramey
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// not actually used here - but needed for integer arithmetic
// so this is a good place to include it
#include "checked_integer.hpp"
#include "checked_result_operations.hpp"

#include "safe_base.hpp"
#include "safe_base_operations.hpp"

#include "native.hpp"
#include "exception_policies.hpp"

// specialization for meta functions with safe<T> argument
namespace boost {
namespace safe_numerics {

template <
    class T,
    class P = native,
    class E = default_exception_policy
>
using safe = safe_base<
    T,
    ::std::numeric_limits<T>::min(),
    ::std::numeric_limits<T>::max(),
    P,
    E
>;

} // safe_numerics
} // boost


#endif // BOOST_NUMERIC_SAFE_INTEGER_HPP
