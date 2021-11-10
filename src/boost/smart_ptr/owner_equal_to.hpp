#ifndef BOOST_SMART_PTR_OWNER_EQUAL_TO_HPP_INCLUDED
#define BOOST_SMART_PTR_OWNER_EQUAL_TO_HPP_INCLUDED

// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>

namespace boost
{

template<class T = void> struct owner_equal_to
{
    typedef bool result_type;
    typedef T first_argument_type;
    typedef T second_argument_type;

    template<class U, class V> bool operator()( U const & u, V const & v ) const BOOST_NOEXCEPT
    {
        return u.owner_equals( v );
    }
};

} // namespace boost

#endif  // #ifndef BOOST_SMART_PTR_OWNER_EQUAL_TO_HPP_INCLUDED
