
//  Copyright (c) 2011 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_BIG_CONSTANT_HPP
#define BOOST_MATH_TOOLS_BIG_CONSTANT_HPP

#include <boost/math/tools/config.hpp>
#include <boost/math/tools/lexical_cast.hpp>

#include <type_traits>
#include <limits>

namespace boost{ namespace math{ 

namespace tools{

template <class T>
struct numeric_traits : public std::numeric_limits< T > {};

#ifdef BOOST_MATH_USE_FLOAT128
typedef __float128 largest_float;
#define BOOST_MATH_LARGEST_FLOAT_C(x) x##Q
template <>
struct numeric_traits<__float128>
{
   static const int digits = 113;
   static const int digits10 = 33;
   static const int max_exponent = 16384;
   static const bool is_specialized = true;
};
#elif LDBL_DIG > DBL_DIG
typedef long double largest_float;
#define BOOST_MATH_LARGEST_FLOAT_C(x) x##L
#else
typedef double largest_float;
#define BOOST_MATH_LARGEST_FLOAT_C(x) x
#endif

template <class T>
inline constexpr T make_big_value(largest_float v, const char*, std::true_type const&, std::false_type const&) BOOST_MATH_NOEXCEPT(T)
{
   return static_cast<T>(v);
}
template <class T>
inline constexpr T make_big_value(largest_float v, const char*, std::true_type const&, std::true_type const&) BOOST_MATH_NOEXCEPT(T)
{
   return static_cast<T>(v);
}
#ifndef BOOST_MATH_NO_LEXICAL_CAST
template <class T>
inline T make_big_value(largest_float, const char* s, std::false_type const&, std::false_type const&)
{
   return boost::lexical_cast<T>(s);
}
#endif
template <class T>
inline constexpr T make_big_value(largest_float, const char* s, std::false_type const&, std::true_type const&) BOOST_MATH_NOEXCEPT(T)
{
   return T(s);
}

//
// For constants which might fit in a long double (if it's big enough):
//
#define BOOST_MATH_BIG_CONSTANT(T, D, x)\
   boost::math::tools::make_big_value<T>(\
      BOOST_MATH_LARGEST_FLOAT_C(x), \
      BOOST_STRINGIZE(x), \
      std::integral_constant<bool, (std::is_convertible<boost::math::tools::largest_float, T>::value) && \
      ((D <= boost::math::tools::numeric_traits<boost::math::tools::largest_float>::digits) \
          || std::is_floating_point<T>::value \
          || (boost::math::tools::numeric_traits<T>::is_specialized && \
          (boost::math::tools::numeric_traits<T>::digits10 <= boost::math::tools::numeric_traits<boost::math::tools::largest_float>::digits10))) >(), \
      std::is_constructible<T, const char*>())
//
// For constants too huge for any conceivable long double (and which generate compiler errors if we try and declare them as such):
//
#define BOOST_MATH_HUGE_CONSTANT(T, D, x)\
   boost::math::tools::make_big_value<T>(0.0L, BOOST_STRINGIZE(x), \
   std::integral_constant<bool, std::is_floating_point<T>::value || (boost::math::tools::numeric_traits<T>::is_specialized && boost::math::tools::numeric_traits<T>::max_exponent <= boost::math::tools::numeric_traits<boost::math::tools::largest_float>::max_exponent && boost::math::tools::numeric_traits<T>::digits <= boost::math::tools::numeric_traits<boost::math::tools::largest_float>::digits)>(), \
   std::is_constructible<T, const char*>())

}}} // namespaces

#endif

