// Copyright Daniel Wallin 2006.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PARAMETER_AUX_AS_LVALUE_HPP
#define BOOST_PARAMETER_AUX_AS_LVALUE_HPP

namespace boost { namespace parameter { namespace aux {

    template <typename T>
    T const& as_lvalue(T const& value)
    {
        return value;
    }

    template <typename T>
    T& as_lvalue(T& value)
    {
        return value;
    }
}}} // namespace boost::parameter::aux

#endif  // include guard

