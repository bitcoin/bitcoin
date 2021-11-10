/*
Copyright 2018 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_IS_BOUNDED_ARRAY_HPP_INCLUDED
#define BOOST_TT_IS_BOUNDED_ARRAY_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>
#include <cstddef>

namespace boost {

template<class T>
struct is_bounded_array
    : false_type { };

#if !defined(BOOST_NO_ARRAY_TYPE_SPECIALIZATIONS)
template<class T, std::size_t N>
struct is_bounded_array<T[N]>
    : true_type { };

template<class T, std::size_t N>
struct is_bounded_array<const T[N]>
    : true_type { };

template<class T, std::size_t N>
struct is_bounded_array<volatile T[N]>
    : true_type { };

template<class T, std::size_t N>
struct is_bounded_array<const volatile T[N]>
    : true_type { };
#endif

} /* boost */

#endif
