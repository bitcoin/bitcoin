/*
Copyright 2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_UNSCOPED_ENUM_HPP_INCLUDED
#define BOOST_TT_IS_UNSCOPED_ENUM_HPP_INCLUDED

#include <boost/type_traits/conjunction.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_convertible.hpp>

namespace boost {

template<class T>
struct is_unscoped_enum
    : conjunction<is_enum<T>, is_convertible<T, int> >::type { };

} /* boost */

#endif
