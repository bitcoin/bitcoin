// Copyright 2005 Alexander Nasonov.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef FILE_boost_type_traits_floating_point_promotion_hpp_INCLUDED
#define FILE_boost_type_traits_floating_point_promotion_hpp_INCLUDED

#include <boost/config.hpp>

namespace boost {

   template<class T> struct floating_point_promotion { typedef T type; };
   template<> struct floating_point_promotion<float> { typedef double type; };
   template<> struct floating_point_promotion<float const> { typedef double const type; };
   template<> struct floating_point_promotion<float volatile>{ typedef double volatile type; };
   template<> struct floating_point_promotion<float const volatile> { typedef double const volatile type; };

#if !defined(BOOST_NO_CXX11_TEMPLATE_ALIASES)

   template <class T> using floating_point_promotion_t = typename floating_point_promotion<T>::type;

#endif

}

#endif // #ifndef FILE_boost_type_traits_floating_point_promotion_hpp_INCLUDED

