
//  (C) Copyright Dave Abrahams, Steve Cleary, Beman Dawes, 
//      Howard Hinnant and John Maddock 2000. 
//  (C) Copyright Mat Marcus, Jesse Jones and Adobe Systems Inc 2001

//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

//    Fixed is_pointer, is_reference, is_const, is_volatile, is_same, 
//    is_member_pointer based on the Simulated Partial Specialization work 
//    of Mat Marcus and Jesse Jones. See  http://opensource.adobe.com or 
//    http://groups.yahoo.com/group/boost/message/5441 
//    Some workarounds in here use ideas suggested from "Generic<Programming>: 
//    Mappings between Types and Values" 
//    by Andrei Alexandrescu (see http://www.cuj.com/experts/1810/alexandr.html).


#ifndef BOOST_TT_IS_POINTER_HPP_INCLUDED
#define BOOST_TT_IS_POINTER_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>

namespace boost {

#if defined( BOOST_CODEGEARC )
template <class T> struct is_pointer : public integral_constant<bool, __is_pointer(T)>{};
#else
template <class T> struct is_pointer : public false_type{};
template <class T> struct is_pointer<T*> : public true_type{};
template <class T> struct is_pointer<T*const> : public true_type{};
template <class T> struct is_pointer<T*const volatile> : public true_type{};
template <class T> struct is_pointer<T*volatile> : public true_type{};

#ifdef BOOST_MSVC
template <class T> struct is_pointer<T const> : public is_pointer<T>{};
template <class T> struct is_pointer<T const volatile> : public is_pointer<T>{};
template <class T> struct is_pointer<T volatile> : public is_pointer<T>{};
#endif

#endif

} // namespace boost

#endif // BOOST_TT_IS_POINTER_HPP_INCLUDED
