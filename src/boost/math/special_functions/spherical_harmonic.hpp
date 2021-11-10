
//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_SPHERICAL_HARMONIC_HPP
#define BOOST_MATH_SPECIAL_SPHERICAL_HARMONIC_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/legendre.hpp>
#include <boost/math/tools/workaround.hpp>
#include <complex>

namespace boost{
namespace math{

namespace detail{

//
// Calculates the prefix term that's common to the real
// and imaginary parts.  Does *not* fix up the sign of the result
// though.
//
template <class T, class Policy>
inline T spherical_harmonic_prefix(unsigned n, unsigned m, T theta, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(m > n)
      return 0;

   T sin_theta = sin(theta);
   T x = cos(theta);

   T leg = detail::legendre_p_imp(n, m, x, static_cast<T>(pow(fabs(sin_theta), T(m))), pol);
   
   T prefix = boost::math::tgamma_delta_ratio(static_cast<T>(n - m + 1), static_cast<T>(2 * m), pol);
   prefix *= (2 * n + 1) / (4 * constants::pi<T>());
   prefix = sqrt(prefix);
   return prefix * leg;
}
//
// Real Part:
//
template <class T, class Policy>
T spherical_harmonic_r(unsigned n, int m, T theta, T phi, const Policy& pol)
{
   BOOST_MATH_STD_USING  // ADL of std functions

   bool sign = false;
   if(m < 0)
   {
      // Reflect and adjust sign if m < 0:
      sign = m&1;
      m = abs(m);
   }
   if(m&1)
   {
      // Check phase if theta is outside [0, PI]:
      T mod = boost::math::tools::fmod_workaround(theta, T(2 * constants::pi<T>()));
      if(mod < 0)
         mod += 2 * constants::pi<T>();
      if(mod > constants::pi<T>())
         sign = !sign;
   }
   // Get the value and adjust sign as required:
   T prefix = spherical_harmonic_prefix(n, m, theta, pol);
   prefix *= cos(m * phi);
   return sign ? T(-prefix) : prefix;
}

template <class T, class Policy>
T spherical_harmonic_i(unsigned n, int m, T theta, T phi, const Policy& pol)
{
   BOOST_MATH_STD_USING  // ADL of std functions

   bool sign = false;
   if(m < 0)
   {
      // Reflect and adjust sign if m < 0:
      sign = !(m&1);
      m = abs(m);
   }
   if(m&1)
   {
      // Check phase if theta is outside [0, PI]:
      T mod = boost::math::tools::fmod_workaround(theta, T(2 * constants::pi<T>()));
      if(mod < 0)
         mod += 2 * constants::pi<T>();
      if(mod > constants::pi<T>())
         sign = !sign;
   }
   // Get the value and adjust sign as required:
   T prefix = spherical_harmonic_prefix(n, m, theta, pol);
   prefix *= sin(m * phi);
   return sign ? T(-prefix) : prefix;
}

template <class T, class U, class Policy>
std::complex<T> spherical_harmonic(unsigned n, int m, U theta, U phi, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Sort out the signs:
   //
   bool r_sign = false;
   bool i_sign = false;
   if(m < 0)
   {
      // Reflect and adjust sign if m < 0:
      r_sign = m&1;
      i_sign = !(m&1);
      m = abs(m);
   }
   if(m&1)
   {
      // Check phase if theta is outside [0, PI]:
      U mod = boost::math::tools::fmod_workaround(theta, U(2 * constants::pi<U>()));
      if(mod < 0)
         mod += 2 * constants::pi<U>();
      if(mod > constants::pi<U>())
      {
         r_sign = !r_sign;
         i_sign = !i_sign;
      }
   }
   //
   // Calculate the value:
   //
   U prefix = spherical_harmonic_prefix(n, m, theta, pol);
   U r = prefix * cos(m * phi);
   U i = prefix * sin(m * phi);
   //
   // Add in the signs:
   //
   if(r_sign)
      r = -r;
   if(i_sign)
      i = -i;
   static const char* function = "boost::math::spherical_harmonic<%1%>(int, int, %1%, %1%)";
   return std::complex<T>(policies::checked_narrowing_cast<T, Policy>(r, function), policies::checked_narrowing_cast<T, Policy>(i, function));
}

} // namespace detail

template <class T1, class T2, class Policy>
inline std::complex<typename tools::promote_args<T1, T2>::type> 
   spherical_harmonic(unsigned n, int m, T1 theta, T2 phi, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return detail::spherical_harmonic<result_type, value_type>(n, m, static_cast<value_type>(theta), static_cast<value_type>(phi), pol);
}

template <class T1, class T2>
inline std::complex<typename tools::promote_args<T1, T2>::type> 
   spherical_harmonic(unsigned n, int m, T1 theta, T2 phi)
{
   return boost::math::spherical_harmonic(n, m, theta, phi, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   spherical_harmonic_r(unsigned n, int m, T1 theta, T2 phi, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::spherical_harmonic_r(n, m, static_cast<value_type>(theta), static_cast<value_type>(phi), pol), "bost::math::spherical_harmonic_r<%1%>(unsigned, int, %1%, %1%)");
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   spherical_harmonic_r(unsigned n, int m, T1 theta, T2 phi)
{
   return boost::math::spherical_harmonic_r(n, m, theta, phi, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   spherical_harmonic_i(unsigned n, int m, T1 theta, T2 phi, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::spherical_harmonic_i(n, m, static_cast<value_type>(theta), static_cast<value_type>(phi), pol), "boost::math::spherical_harmonic_i<%1%>(unsigned, int, %1%, %1%)");
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   spherical_harmonic_i(unsigned n, int m, T1 theta, T2 phi)
{
   return boost::math::spherical_harmonic_i(n, m, theta, phi, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SPECIAL_SPHERICAL_HARMONIC_HPP



