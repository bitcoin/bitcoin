
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


#ifndef BOOST_TT_IS_SAME_HPP_INCLUDED
#define BOOST_TT_IS_SAME_HPP_INCLUDED

#include <boost/type_traits/integral_constant.hpp>

namespace boost {


   template <class T, class U> struct is_same : public false_type {};
   template <class T> struct is_same<T,T> : public true_type {};
#if BOOST_WORKAROUND(BOOST_BORLANDC, < 0x600)
// without this, Borland's compiler gives the wrong answer for
// references to arrays:
   template <class T> struct is_same<T&, T&> : public true_type{};
#endif


} // namespace boost

#endif  // BOOST_TT_IS_SAME_HPP_INCLUDED

