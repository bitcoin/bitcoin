//  Copyright John Maddock 2018.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//
// Tools for operator on complex as well as scalar types.
//

#ifndef BOOST_MATH_TOOLS_COMPLEX_HPP
#define BOOST_MATH_TOOLS_COMPLEX_HPP

#include <utility>
#include <boost/math/tools/is_detected.hpp>

namespace boost {
   namespace math {
      namespace tools {

         namespace detail {
         template <typename T, typename = void>
         struct is_complex_type_impl
         {
            static constexpr bool value = false;
         };

         template <typename T>
         struct is_complex_type_impl<T, void_t<decltype(std::declval<T>().real()), 
                                               decltype(std::declval<T>().imag())>>
         {
            static constexpr bool value = true;
         };
         } // Namespace detail

         template <typename T>
         struct is_complex_type : public detail::is_complex_type_impl<T> {};
         
         //
         // Use this trait to typecast integer literals to something
         // that will interoperate with T:
         //
         template <class T, bool = is_complex_type<T>::value>
         struct integer_scalar_type
         {
            typedef int type;
         };
         template <class T>
         struct integer_scalar_type<T, true>
         {
            typedef typename T::value_type type;
         };
         template <class T, bool = is_complex_type<T>::value>
         struct unsigned_scalar_type
         {
            typedef unsigned type;
         };
         template <class T>
         struct unsigned_scalar_type<T, true>
         {
            typedef typename T::value_type type;
         };
         template <class T, bool = is_complex_type<T>::value>
         struct scalar_type
         {
            typedef T type;
         };
         template <class T>
         struct scalar_type<T, true>
         {
            typedef typename T::value_type type;
         };


} } }

#endif // BOOST_MATH_TOOLS_COMPLEX_HPP
