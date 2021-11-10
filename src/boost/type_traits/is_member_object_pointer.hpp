
//  (C) Copyright John Maddock 2005.  
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.


#ifndef BOOST_TT_IS_MEMBER_OBJECT_POINTER_HPP_INCLUDED
#define BOOST_TT_IS_MEMBER_OBJECT_POINTER_HPP_INCLUDED

#include <boost/type_traits/is_member_pointer.hpp>
#include <boost/type_traits/is_member_function_pointer.hpp>

namespace boost {

template <class T> struct is_member_object_pointer 
   : public integral_constant<bool, 
   ::boost::is_member_pointer<T>::value && !::boost::is_member_function_pointer<T>::value>{};

} // namespace boost

#endif // BOOST_TT_IS_MEMBER_FUNCTION_POINTER_HPP_INCLUDED
