/*
Copyright 2020 Glen Joseph Fernandes
(glenjofe@gmail.com)

Distributed under the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt
or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_TT_DISJUNCTION_HPP_INCLUDED
#define BOOST_TT_DISJUNCTION_HPP_INCLUDED

#include <boost/type_traits/conditional.hpp>
#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
#include <boost/type_traits/integral_constant.hpp>
#endif

namespace boost {

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
template<class...>
struct disjunction
    : false_type { };

template<class T>
struct disjunction<T>
    : T { };

template<class T, class... U>
struct disjunction<T, U...>
    : conditional<bool(T::value), T, disjunction<U...> >::type { };
#else
template<class T, class U>
struct disjunction
    : conditional<bool(T::value), T, U>::type { };
#endif

} /* boost */

#endif
