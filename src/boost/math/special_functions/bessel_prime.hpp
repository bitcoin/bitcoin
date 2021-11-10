//  Copyright (c) 2013 Anton Bikineev
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_BESSEL_DERIVATIVES_HPP
#define BOOST_MATH_BESSEL_DERIVATIVES_HPP

#ifdef _MSC_VER
#  pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/bessel.hpp>
#include <boost/math/special_functions/detail/bessel_jy_derivatives_asym.hpp>
#include <boost/math/special_functions/detail/bessel_jy_derivatives_series.hpp>
#include <boost/math/special_functions/detail/bessel_derivatives_linear.hpp>

namespace boost{ namespace math{

namespace detail{

template <class Tag, class T, class Policy>
inline T cyl_bessel_j_prime_imp(T v, T x, const Policy& pol)
{
   static const char* const function = "boost::math::cyl_bessel_j_prime<%1%>(%1%,%1%)";
   BOOST_MATH_STD_USING
   //
   // Prevent complex result:
   //
   if (x < 0 && floor(v) != v)
      return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function requires x >= 0", x, pol);
   //
   // Special cases for x == 0:
   //
   if (x == 0)
   {
      if (v == 1)
         return 0.5;
      else if (v == -1)
         return -0.5;
      else if (floor(v) == v || v > 1)
         return 0;
      else return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function is indeterminate for this order", x, pol);
   }
   //
   // Special case for large x: use asymptotic expansion:
   //
   if (boost::math::detail::asymptotic_bessel_derivative_large_x_limit(v, x))
      return boost::math::detail::asymptotic_bessel_j_derivative_large_x_2(v, x, pol);
   //
   // Special case for small x: use Taylor series:
   //
   if ((abs(x) < 5) || (abs(v) > x * x / 4))
   {
      bool inversed = false;
      if (floor(v) == v && v < 0)
      {
         v = -v;
         if (itrunc(v, pol) & 1)
            inversed = true;
      }
      T r = boost::math::detail::bessel_j_derivative_small_z_series(v, x, pol);
      return inversed ? T(-r) : r;
   }
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return -boost::math::detail::cyl_bessel_j_imp<T>(1, x, Tag(), pol);
   //
   // Default case:
   //
   return boost::math::detail::bessel_j_derivative_linear(v, x, Tag(), pol);
}

template <class T, class Policy>
inline T sph_bessel_j_prime_imp(unsigned v, T x, const Policy& pol)
{
   static const char* const function = "boost::math::sph_bessel_prime<%1%>(%1%,%1%)";
   //
   // Prevent complex result:
   //
   if (x < 0)
      return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function requires x >= 0.", x, pol);
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return (x == 0) ? boost::math::policies::raise_overflow_error<T>(function, 0, pol)
         : static_cast<T>(-boost::math::detail::sph_bessel_j_imp<T>(1, x, pol));
   //
   // Special case for x == 0 and v > 0:
   //
   if (x == 0)
      return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function is indeterminate for this order", x, pol);
   //
   // Default case:
   //
   return boost::math::detail::sph_bessel_j_derivative_linear(v, x, pol);
}

template <class T, class Policy>
inline T cyl_bessel_i_prime_imp(T v, T x, const Policy& pol)
{
   static const char* const function = "boost::math::cyl_bessel_i_prime<%1%>(%1%,%1%)";
   BOOST_MATH_STD_USING
   //
   // Prevent complex result:
   //
   if (x < 0 && floor(v) != v)
      return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function requires x >= 0", x, pol);
   //
   // Special cases for x == 0:
   //
   if (x == 0)
   {
      if (v == 1 || v == -1)
         return 0.5;
      else if (floor(v) == v || v > 1)
         return 0;
      else return boost::math::policies::raise_domain_error<T>(
         function,
         "Got x = %1%, but function is indeterminate for this order", x, pol);
   }
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return boost::math::detail::cyl_bessel_i_imp<T>(1, x, pol);
   //
   // Default case:
   //
   return boost::math::detail::bessel_i_derivative_linear(v, x, pol);
}

template <class Tag, class T, class Policy>
inline T cyl_bessel_k_prime_imp(T v, T x, const Policy& pol)
{
   //
   // Prevent complex and indeterminate results:
   //
   if (x <= 0)
      return boost::math::policies::raise_domain_error<T>(
         "boost::math::cyl_bessel_k_prime<%1%>(%1%,%1%)",
         "Got x = %1%, but function requires x > 0", x, pol);
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return -boost::math::detail::cyl_bessel_k_imp<T>(1, x, Tag(), pol);
   //
   // Default case:
   //
   return boost::math::detail::bessel_k_derivative_linear(v, x, Tag(), pol);
}

template <class Tag, class T, class Policy>
inline T cyl_neumann_prime_imp(T v, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Prevent complex and indeterminate results:
   //
   if (x <= 0)
      return boost::math::policies::raise_domain_error<T>(
         "boost::math::cyl_neumann_prime<%1%>(%1%,%1%)",
         "Got x = %1%, but function requires x > 0", x, pol);
   //
   // Special case for large x: use asymptotic expansion:
   //
   if (boost::math::detail::asymptotic_bessel_derivative_large_x_limit(v, x))
      return boost::math::detail::asymptotic_bessel_y_derivative_large_x_2(v, x, pol);
   //
   // Special case for small x: use Taylor series:
   //
   if (v > 0 && floor(v) != v)
   {
      const T eps = boost::math::policies::get_epsilon<T, Policy>();
      if (log(eps / 2) > v * log((x * x) / (v * 4)))
         return boost::math::detail::bessel_y_derivative_small_z_series(v, x, pol);
   }
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return -boost::math::detail::cyl_neumann_imp<T>(1, x, Tag(), pol);
   //
   // Default case:
   //
   return boost::math::detail::bessel_y_derivative_linear(v, x, Tag(), pol);
}

template <class T, class Policy>
inline T sph_neumann_prime_imp(unsigned v, T x, const Policy& pol)
{
   //
   // Prevent complex and indeterminate result:
   //
   if (x <= 0)
      return boost::math::policies::raise_domain_error<T>(
         "boost::math::sph_neumann_prime<%1%>(%1%,%1%)",
         "Got x = %1%, but function requires x > 0.", x, pol);
   //
   // Special case for v == 0:
   //
   if (v == 0)
      return -boost::math::detail::sph_neumann_imp<T>(1, x, pol);
   //
   // Default case:
   //
   return boost::math::detail::sph_neumann_derivative_linear(v, x, pol);
}

} // namespace detail

template <class T1, class T2, class Policy>
inline typename detail::bessel_traits<T1, T2, Policy>::result_type cyl_bessel_j_prime(T1 v, T2 x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename detail::bessel_traits<T1, T2, Policy>::optimisation_tag tag_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::cyl_bessel_j_prime_imp<tag_type, value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy()), "boost::math::cyl_bessel_j_prime<%1%,%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type cyl_bessel_j_prime(T1 v, T2 x)
{
   return cyl_bessel_j_prime(v, x, policies::policy<>());
}

template <class T, class Policy>
inline typename detail::bessel_traits<T, T, Policy>::result_type sph_bessel_prime(unsigned v, T x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T, T, Policy>::result_type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::sph_bessel_j_prime_imp<value_type>(v, static_cast<value_type>(x), forwarding_policy()), "boost::math::sph_bessel_j_prime<%1%>(%1%,%1%)");
}

template <class T>
inline typename detail::bessel_traits<T, T, policies::policy<> >::result_type sph_bessel_prime(unsigned v, T x)
{
   return sph_bessel_prime(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename detail::bessel_traits<T1, T2, Policy>::result_type cyl_bessel_i_prime(T1 v, T2 x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::cyl_bessel_i_prime_imp<value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy()), "boost::math::cyl_bessel_i_prime<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type cyl_bessel_i_prime(T1 v, T2 x)
{
   return cyl_bessel_i_prime(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename detail::bessel_traits<T1, T2, Policy>::result_type cyl_bessel_k_prime(T1 v, T2 x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename detail::bessel_traits<T1, T2, Policy>::optimisation_tag tag_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::cyl_bessel_k_prime_imp<tag_type, value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy()), "boost::math::cyl_bessel_k_prime<%1%,%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type cyl_bessel_k_prime(T1 v, T2 x)
{
   return cyl_bessel_k_prime(v, x, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename detail::bessel_traits<T1, T2, Policy>::result_type cyl_neumann_prime(T1 v, T2 x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T1, T2, Policy>::result_type result_type;
   typedef typename detail::bessel_traits<T1, T2, Policy>::optimisation_tag tag_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::cyl_neumann_prime_imp<tag_type, value_type>(static_cast<value_type>(v), static_cast<value_type>(x), forwarding_policy()), "boost::math::cyl_neumann_prime<%1%,%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline typename detail::bessel_traits<T1, T2, policies::policy<> >::result_type cyl_neumann_prime(T1 v, T2 x)
{
   return cyl_neumann_prime(v, x, policies::policy<>());
}

template <class T, class Policy>
inline typename detail::bessel_traits<T, T, Policy>::result_type sph_neumann_prime(unsigned v, T x, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename detail::bessel_traits<T, T, Policy>::result_type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(detail::sph_neumann_prime_imp<value_type>(v, static_cast<value_type>(x), forwarding_policy()), "boost::math::sph_neumann_prime<%1%>(%1%,%1%)");
}

template <class T>
inline typename detail::bessel_traits<T, T, policies::policy<> >::result_type sph_neumann_prime(unsigned v, T x)
{
   return sph_neumann_prime(v, x, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_BESSEL_DERIVATIVES_HPP
