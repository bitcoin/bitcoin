/*
Copyright 2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_CONJUNCTION_HPP_INCLUDED
#define BOOST_TT_CONJUNCTION_HPP_INCLUDED

#include <boost/type_traits/conditional.hpp>
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/type_traits/integral_constant.hpp>
#endif

namespace boost {

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<class...>
struct conjunction
    : true_type { };

template<class T>
struct conjunction<T>
    : T { };

template<class T, class... U>
struct conjunction<T, U...>
    : conditional<bool(T::value), conjunction<U...>, T>::type { };
#else
template<class T, class U>
struct conjunction
    : conditional<bool(T::value), U, T>::type { };
#endif

} /* boost */

#endif
