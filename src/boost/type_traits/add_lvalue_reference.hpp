//  Copyright 2010 John Maddock

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_TYPE_TRAITS_EXT_ADD_LVALUE_REFERENCE__HPP
#define BOOST_TYPE_TRAITS_EXT_ADD_LVALUE_REFERENCE__HPP

#include <boost/type_traits/add_reference.hpp>

namespace boost{

template <class T> struct add_lvalue_reference
{
   typedef typename boost::add_reference<T>::type type; 
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
template <class T> struct add_lvalue_reference<T&&>
{
   typedef T& type;
};
#endif

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;

#endif

}

#endif  // BOOST_TYPE_TRAITS_EXT_ADD_LVALUE_REFERENCE__HPP
