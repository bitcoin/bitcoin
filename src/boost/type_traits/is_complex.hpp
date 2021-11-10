//  (C) Copyright John Maddock 2007. 
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).
//
//  See http://www.boost.org/libs/type_traits for most recent version including documentation.

#ifndef BOOST_TT_IS_COMPLEX_HPP
#define BOOST_TT_IS_COMPLEX_HPP

#include <boost/config.hpp>
#include <complex>
#include <boost/type_traits/integral_constant.hpp>

namespace boost {

   template <class T> struct is_complex : public false_type {};
   template <class T> struct is_complex<const T > : public is_complex<T>{};
   template <class T> struct is_complex<volatile const T > : public is_complex<T>{};
   template <class T> struct is_complex<volatile T > : public is_complex<T>{};
   template <class T> struct is_complex<std::complex<T> > : public true_type{};

} // namespace boost

#endif //BOOST_TT_IS_COMPLEX_HPP
