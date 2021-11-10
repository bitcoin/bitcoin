
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, Howard
//  Hinnant & John Maddock 2000.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_IS_UNION_HPP_INCLUDED
#define BOOST_TT_IS_UNION_HPP_INCLUDED

#include <boost/type_traits/intrinsics.hpp>
#include <boost/type_traits/integral_constant.hpp>

namespace boost {

#ifdef BOOST_IS_UNION
template <class T> struct is_union : public integral_constant<bool, BOOST_IS_UNION(T)> {};
#else
template <class T> struct is_union : public integral_constant<bool, false> {};
#endif

template <class T> struct is_union<T const> : public is_union<T>{};
template <class T> struct is_union<T volatile const> : public is_union<T>{};
template <class T> struct is_union<T volatile> : public is_union<T>{};

} // namespace boost

#endif // BOOST_TT_IS_UNION_HPP_INCLUDED
