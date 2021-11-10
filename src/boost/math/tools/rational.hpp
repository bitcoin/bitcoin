//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_RATIONAL_HPP
#define BOOST_MATH_TOOLS_RATIONAL_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <array>
#include <boost/math/tools/config.hpp>
#include <boost/math/tools/assert.hpp>

#if BOOST_MATH_POLY_METHOD == 1
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/polynomial_horner1_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#elif BOOST_MATH_POLY_METHOD == 2
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/polynomial_horner2_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#elif BOOST_MATH_POLY_METHOD == 3
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/polynomial_horner3_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#endif
#if BOOST_MATH_RATIONAL_METHOD == 1
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/rational_horner1_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#elif BOOST_MATH_RATIONAL_METHOD == 2
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/rational_horner2_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#elif BOOST_MATH_RATIONAL_METHOD == 3
#  define BOOST_HEADER() <BOOST_JOIN(boost/math/tools/detail/rational_horner3_, BOOST_MATH_MAX_POLY_ORDER).hpp>
#  include BOOST_HEADER()
#  undef BOOST_HEADER
#endif

#if 0
//
// This just allows dependency trackers to find the headers
// used in the above PP-magic.
//
#include <boost/math/tools/detail/polynomial_horner1_2.hpp>
#include <boost/math/tools/detail/polynomial_horner1_3.hpp>
#include <boost/math/tools/detail/polynomial_horner1_4.hpp>
#include <boost/math/tools/detail/polynomial_horner1_5.hpp>
#include <boost/math/tools/detail/polynomial_horner1_6.hpp>
#include <boost/math/tools/detail/polynomial_horner1_7.hpp>
#include <boost/math/tools/detail/polynomial_horner1_8.hpp>
#include <boost/math/tools/detail/polynomial_horner1_9.hpp>
#include <boost/math/tools/detail/polynomial_horner1_10.hpp>
#include <boost/math/tools/detail/polynomial_horner1_11.hpp>
#include <boost/math/tools/detail/polynomial_horner1_12.hpp>
#include <boost/math/tools/detail/polynomial_horner1_13.hpp>
#include <boost/math/tools/detail/polynomial_horner1_14.hpp>
#include <boost/math/tools/detail/polynomial_horner1_15.hpp>
#include <boost/math/tools/detail/polynomial_horner1_16.hpp>
#include <boost/math/tools/detail/polynomial_horner1_17.hpp>
#include <boost/math/tools/detail/polynomial_horner1_18.hpp>
#include <boost/math/tools/detail/polynomial_horner1_19.hpp>
#include <boost/math/tools/detail/polynomial_horner1_20.hpp>
#include <boost/math/tools/detail/polynomial_horner2_2.hpp>
#include <boost/math/tools/detail/polynomial_horner2_3.hpp>
#include <boost/math/tools/detail/polynomial_horner2_4.hpp>
#include <boost/math/tools/detail/polynomial_horner2_5.hpp>
#include <boost/math/tools/detail/polynomial_horner2_6.hpp>
#include <boost/math/tools/detail/polynomial_horner2_7.hpp>
#include <boost/math/tools/detail/polynomial_horner2_8.hpp>
#include <boost/math/tools/detail/polynomial_horner2_9.hpp>
#include <boost/math/tools/detail/polynomial_horner2_10.hpp>
#include <boost/math/tools/detail/polynomial_horner2_11.hpp>
#include <boost/math/tools/detail/polynomial_horner2_12.hpp>
#include <boost/math/tools/detail/polynomial_horner2_13.hpp>
#include <boost/math/tools/detail/polynomial_horner2_14.hpp>
#include <boost/math/tools/detail/polynomial_horner2_15.hpp>
#include <boost/math/tools/detail/polynomial_horner2_16.hpp>
#include <boost/math/tools/detail/polynomial_horner2_17.hpp>
#include <boost/math/tools/detail/polynomial_horner2_18.hpp>
#include <boost/math/tools/detail/polynomial_horner2_19.hpp>
#include <boost/math/tools/detail/polynomial_horner2_20.hpp>
#include <boost/math/tools/detail/polynomial_horner3_2.hpp>
#include <boost/math/tools/detail/polynomial_horner3_3.hpp>
#include <boost/math/tools/detail/polynomial_horner3_4.hpp>
#include <boost/math/tools/detail/polynomial_horner3_5.hpp>
#include <boost/math/tools/detail/polynomial_horner3_6.hpp>
#include <boost/math/tools/detail/polynomial_horner3_7.hpp>
#include <boost/math/tools/detail/polynomial_horner3_8.hpp>
#include <boost/math/tools/detail/polynomial_horner3_9.hpp>
#include <boost/math/tools/detail/polynomial_horner3_10.hpp>
#include <boost/math/tools/detail/polynomial_horner3_11.hpp>
#include <boost/math/tools/detail/polynomial_horner3_12.hpp>
#include <boost/math/tools/detail/polynomial_horner3_13.hpp>
#include <boost/math/tools/detail/polynomial_horner3_14.hpp>
#include <boost/math/tools/detail/polynomial_horner3_15.hpp>
#include <boost/math/tools/detail/polynomial_horner3_16.hpp>
#include <boost/math/tools/detail/polynomial_horner3_17.hpp>
#include <boost/math/tools/detail/polynomial_horner3_18.hpp>
#include <boost/math/tools/detail/polynomial_horner3_19.hpp>
#include <boost/math/tools/detail/polynomial_horner3_20.hpp>
#include <boost/math/tools/detail/rational_horner1_2.hpp>
#include <boost/math/tools/detail/rational_horner1_3.hpp>
#include <boost/math/tools/detail/rational_horner1_4.hpp>
#include <boost/math/tools/detail/rational_horner1_5.hpp>
#include <boost/math/tools/detail/rational_horner1_6.hpp>
#include <boost/math/tools/detail/rational_horner1_7.hpp>
#include <boost/math/tools/detail/rational_horner1_8.hpp>
#include <boost/math/tools/detail/rational_horner1_9.hpp>
#include <boost/math/tools/detail/rational_horner1_10.hpp>
#include <boost/math/tools/detail/rational_horner1_11.hpp>
#include <boost/math/tools/detail/rational_horner1_12.hpp>
#include <boost/math/tools/detail/rational_horner1_13.hpp>
#include <boost/math/tools/detail/rational_horner1_14.hpp>
#include <boost/math/tools/detail/rational_horner1_15.hpp>
#include <boost/math/tools/detail/rational_horner1_16.hpp>
#include <boost/math/tools/detail/rational_horner1_17.hpp>
#include <boost/math/tools/detail/rational_horner1_18.hpp>
#include <boost/math/tools/detail/rational_horner1_19.hpp>
#include <boost/math/tools/detail/rational_horner1_20.hpp>
#include <boost/math/tools/detail/rational_horner2_2.hpp>
#include <boost/math/tools/detail/rational_horner2_3.hpp>
#include <boost/math/tools/detail/rational_horner2_4.hpp>
#include <boost/math/tools/detail/rational_horner2_5.hpp>
#include <boost/math/tools/detail/rational_horner2_6.hpp>
#include <boost/math/tools/detail/rational_horner2_7.hpp>
#include <boost/math/tools/detail/rational_horner2_8.hpp>
#include <boost/math/tools/detail/rational_horner2_9.hpp>
#include <boost/math/tools/detail/rational_horner2_10.hpp>
#include <boost/math/tools/detail/rational_horner2_11.hpp>
#include <boost/math/tools/detail/rational_horner2_12.hpp>
#include <boost/math/tools/detail/rational_horner2_13.hpp>
#include <boost/math/tools/detail/rational_horner2_14.hpp>
#include <boost/math/tools/detail/rational_horner2_15.hpp>
#include <boost/math/tools/detail/rational_horner2_16.hpp>
#include <boost/math/tools/detail/rational_horner2_17.hpp>
#include <boost/math/tools/detail/rational_horner2_18.hpp>
#include <boost/math/tools/detail/rational_horner2_19.hpp>
#include <boost/math/tools/detail/rational_horner2_20.hpp>
#include <boost/math/tools/detail/rational_horner3_2.hpp>
#include <boost/math/tools/detail/rational_horner3_3.hpp>
#include <boost/math/tools/detail/rational_horner3_4.hpp>
#include <boost/math/tools/detail/rational_horner3_5.hpp>
#include <boost/math/tools/detail/rational_horner3_6.hpp>
#include <boost/math/tools/detail/rational_horner3_7.hpp>
#include <boost/math/tools/detail/rational_horner3_8.hpp>
#include <boost/math/tools/detail/rational_horner3_9.hpp>
#include <boost/math/tools/detail/rational_horner3_10.hpp>
#include <boost/math/tools/detail/rational_horner3_11.hpp>
#include <boost/math/tools/detail/rational_horner3_12.hpp>
#include <boost/math/tools/detail/rational_horner3_13.hpp>
#include <boost/math/tools/detail/rational_horner3_14.hpp>
#include <boost/math/tools/detail/rational_horner3_15.hpp>
#include <boost/math/tools/detail/rational_horner3_16.hpp>
#include <boost/math/tools/detail/rational_horner3_17.hpp>
#include <boost/math/tools/detail/rational_horner3_18.hpp>
#include <boost/math/tools/detail/rational_horner3_19.hpp>
#include <boost/math/tools/detail/rational_horner3_20.hpp>
#endif

namespace boost{ namespace math{ namespace tools{

//
// Forward declaration to keep two phase lookup happy:
//
template <class T, class U>
U evaluate_polynomial(const T* poly, U const& z, std::size_t count) BOOST_MATH_NOEXCEPT(U);

namespace detail{

template <class T, class V, class Tag>
inline V evaluate_polynomial_c_imp(const T* a, const V& val, const Tag*) BOOST_MATH_NOEXCEPT(V)
{
   return evaluate_polynomial(a, val, Tag::value);
}

} // namespace detail

//
// Polynomial evaluation with runtime size.
// This requires a for-loop which may be more expensive than
// the loop expanded versions above:
//
template <class T, class U>
inline U evaluate_polynomial(const T* poly, U const& z, std::size_t count) BOOST_MATH_NOEXCEPT(U)
{
   BOOST_MATH_ASSERT(count > 0);
   U sum = static_cast<U>(poly[count - 1]);
   for(int i = static_cast<int>(count) - 2; i >= 0; --i)
   {
      sum *= z;
      sum += static_cast<U>(poly[i]);
   }
   return sum;
}
//
// Compile time sized polynomials, just inline forwarders to the
// implementations above:
//
template <std::size_t N, class T, class V>
inline V evaluate_polynomial(const T(&a)[N], const V& val) BOOST_MATH_NOEXCEPT(V)
{
   typedef std::integral_constant<int, N> tag_type;
   return detail::evaluate_polynomial_c_imp(static_cast<const T*>(a), val, static_cast<tag_type const*>(0));
}

template <std::size_t N, class T, class V>
inline V evaluate_polynomial(const std::array<T,N>& a, const V& val) BOOST_MATH_NOEXCEPT(V)
{
   typedef std::integral_constant<int, N> tag_type;
   return detail::evaluate_polynomial_c_imp(static_cast<const T*>(a.data()), val, static_cast<tag_type const*>(0));
}
//
// Even polynomials are trivial: just square the argument!
//
template <class T, class U>
inline U evaluate_even_polynomial(const T* poly, U z, std::size_t count) BOOST_MATH_NOEXCEPT(U)
{
   return evaluate_polynomial(poly, U(z*z), count);
}

template <std::size_t N, class T, class V>
inline V evaluate_even_polynomial(const T(&a)[N], const V& z) BOOST_MATH_NOEXCEPT(V)
{
   return evaluate_polynomial(a, V(z*z));
}

template <std::size_t N, class T, class V>
inline V evaluate_even_polynomial(const std::array<T,N>& a, const V& z) BOOST_MATH_NOEXCEPT(V)
{
   return evaluate_polynomial(a, V(z*z));
}
//
// Odd polynomials come next:
//
template <class T, class U>
inline U evaluate_odd_polynomial(const T* poly, U z, std::size_t count) BOOST_MATH_NOEXCEPT(U)
{
   return poly[0] + z * evaluate_polynomial(poly+1, U(z*z), count-1);
}

template <std::size_t N, class T, class V>
inline V evaluate_odd_polynomial(const T(&a)[N], const V& z) BOOST_MATH_NOEXCEPT(V)
{
   typedef std::integral_constant<int, N-1> tag_type;
   return a[0] + z * detail::evaluate_polynomial_c_imp(static_cast<const T*>(a) + 1, V(z*z), static_cast<tag_type const*>(0));
}

template <std::size_t N, class T, class V>
inline V evaluate_odd_polynomial(const std::array<T,N>& a, const V& z) BOOST_MATH_NOEXCEPT(V)
{
   typedef std::integral_constant<int, N-1> tag_type;
   return a[0] + z * detail::evaluate_polynomial_c_imp(static_cast<const T*>(a.data()) + 1, V(z*z), static_cast<tag_type const*>(0));
}

template <class T, class U, class V>
V evaluate_rational(const T* num, const U* denom, const V& z_, std::size_t count) BOOST_MATH_NOEXCEPT(V);

namespace detail{

template <class T, class U, class V, class Tag>
inline V evaluate_rational_c_imp(const T* num, const U* denom, const V& z, const Tag*) BOOST_MATH_NOEXCEPT(V)
{
   return boost::math::tools::evaluate_rational(num, denom, z, Tag::value);
}

}
//
// Rational functions: numerator and denominator must be
// equal in size.  These always have a for-loop and so may be less
// efficient than evaluating a pair of polynomials. However, there
// are some tricks we can use to prevent overflow that might otherwise
// occur in polynomial evaluation, if z is large.  This is important
// in our Lanczos code for example.
//
template <class T, class U, class V>
V evaluate_rational(const T* num, const U* denom, const V& z_, std::size_t count) BOOST_MATH_NOEXCEPT(V)
{
   V z(z_);
   V s1, s2;
   if(z <= 1)
   {
      s1 = static_cast<V>(num[count-1]);
      s2 = static_cast<V>(denom[count-1]);
      for(int i = (int)count - 2; i >= 0; --i)
      {
         s1 *= z;
         s2 *= z;
         s1 += num[i];
         s2 += denom[i];
      }
   }
   else
   {
      z = 1 / z;
      s1 = static_cast<V>(num[0]);
      s2 = static_cast<V>(denom[0]);
      for(unsigned i = 1; i < count; ++i)
      {
         s1 *= z;
         s2 *= z;
         s1 += num[i];
         s2 += denom[i];
      }
   }
   return s1 / s2;
}

template <std::size_t N, class T, class U, class V>
inline V evaluate_rational(const T(&a)[N], const U(&b)[N], const V& z) BOOST_MATH_NOEXCEPT(V)
{
   return detail::evaluate_rational_c_imp(a, b, z, static_cast<const std::integral_constant<int, N>*>(0));
}

template <std::size_t N, class T, class U, class V>
inline V evaluate_rational(const std::array<T,N>& a, const std::array<U,N>& b, const V& z) BOOST_MATH_NOEXCEPT(V)
{
   return detail::evaluate_rational_c_imp(a.data(), b.data(), z, static_cast<std::integral_constant<int, N>*>(0));
}

} // namespace tools
} // namespace math
} // namespace boost

#endif // BOOST_MATH_TOOLS_RATIONAL_HPP




