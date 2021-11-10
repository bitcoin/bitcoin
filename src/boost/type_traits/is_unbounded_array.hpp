/*
Copyright 2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_UNBOUNDED_ARRAY_HPP_INCLUDED
#define BOOST_TT_IS_UNBOUNDED_ARRAY_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>

namespace boost {

template<class T>
struct is_unbounded_array
    : false_type { };

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template<class T>
struct is_unbounded_array<T[]>
    : true_type { };

template<class T>
struct is_unbounded_array<const T[]>
    : true_type { };

template<class T>
struct is_unbounded_array<volatile T[]>
    : true_type { };

template<class T>
struct is_unbounded_array<const volatile T[]>
    : true_type { };
#endif

} /* boost */

#endif
