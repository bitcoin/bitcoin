//  Copyright (c) 2011 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_BESSEL_JN_SERIES_HPP
#define BOOST_MATH_BESSEL_JN_SERIES_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <cmath>
#include <cstdint>

namespace boost { namespace math { namespace detail{

template <class T, class Policy>
struct bessel_j_small_z_series_term
{
   typedef T result_type;

   bessel_j_small_z_series_term(T v_, T x)
      : N(0), v(v_)
   {
      BOOST_MATH_STD_USING
      mult = x / 2;
      mult *= -mult;
      term = 1;
   }
   T operator()()
   {
      T r = term;
      ++N;
      term *= mult / (N * (N + v));
      return r;
   }
private:
   unsigned N;
   T v;
   T mult;
   T term;
};
//
// Series evaluation for BesselJ(v, z) as z -> 0.
// See http://functions.wolfram.com/Bessel-TypeFunctions/BesselJ/06/01/04/01/01/0003/
// Converges rapidly for all z << v.
//
template <class T, class Policy>
inline T bessel_j_small_z_series(T v, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   T prefix;
   if(v < max_factorial<T>::value)
   {
      prefix = pow(x / 2, v) / boost::math::tgamma(v+1, pol);
   }
   else
   {
      prefix = v * log(x / 2) - boost::math::lgamma(v+1, pol);
      prefix = exp(prefix);
   }
   if(0 == prefix)
      return prefix;

   bessel_j_small_z_series_term<T, Policy> s(v, x);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();

   T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);

   policies::check_series_iterations<T>("boost::math::bessel_j_small_z_series<%1%>(%1%,%1%)", max_iter, pol);
   return prefix * result;
}

template <class T, class Policy>
struct bessel_y_small_z_series_term_a
{
   typedef T result_type;

   bessel_y_small_z_series_term_a(T v_, T x)
      : N(0), v(v_)
   {
      BOOST_MATH_STD_USING
      mult = x / 2;
      mult *= -mult;
      term = 1;
   }
   T operator()()
   {
      BOOST_MATH_STD_USING
      T r = term;
      ++N;
      term *= mult / (N * (N - v));
      return r;
   }
private:
   unsigned N;
   T v;
   T mult;
   T term;
};

template <class T, class Policy>
struct bessel_y_small_z_series_term_b
{
   typedef T result_type;

   bessel_y_small_z_series_term_b(T v_, T x)
      : N(0), v(v_)
   {
      BOOST_MATH_STD_USING
      mult = x / 2;
      mult *= -mult;
      term = 1;
   }
   T operator()()
   {
      T r = term;
      ++N;
      term *= mult / (N * (N + v));
      return r;
   }
private:
   unsigned N;
   T v;
   T mult;
   T term;
};
//
// Series form for BesselY as z -> 0, 
// see: http://functions.wolfram.com/Bessel-TypeFunctions/BesselY/06/01/04/01/01/0003/
// This series is only useful when the second term is small compared to the first
// otherwise we get catastrophic cancellation errors.
//
// Approximating tgamma(v) by v^v, and assuming |tgamma(-z)| < eps we end up requiring:
// eps/2 * v^v(x/2)^-v > (x/2)^v or log(eps/2) > v log((x/2)^2/v)
//
template <class T, class Policy>
inline T bessel_y_small_z_series(T v, T x, T* pscale, const Policy& pol)
{
   BOOST_MATH_STD_USING
   static const char* function = "bessel_y_small_z_series<%1%>(%1%,%1%)";
   T prefix;
   T gam;
   T p = log(x / 2);
   T scale = 1;
   bool need_logs = (v >= max_factorial<T>::value) || (tools::log_max_value<T>() / v < fabs(p));
   if(!need_logs)
   {
      gam = boost::math::tgamma(v, pol);
      p = pow(x / 2, v);
      if(tools::max_value<T>() * p < gam)
      {
         scale /= gam;
         gam = 1;
         if(tools::max_value<T>() * p < gam)
         {
            return -policies::raise_overflow_error<T>(function, 0, pol);
         }
      }
      prefix = -gam / (constants::pi<T>() * p);
   }
   else
   {
      gam = boost::math::lgamma(v, pol);
      p = v * p;
      prefix = gam - log(constants::pi<T>()) - p;
      if(tools::log_max_value<T>() < prefix)
      {
         prefix -= log(tools::max_value<T>() / 4);
         scale /= (tools::max_value<T>() / 4);
         if(tools::log_max_value<T>() < prefix)
         {
            return -policies::raise_overflow_error<T>(function, 0, pol);
         }
      }
      prefix = -exp(prefix);
   }
   bessel_y_small_z_series_term_a<T, Policy> s(v, x);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   *pscale = scale;

   T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);

   policies::check_series_iterations<T>("boost::math::bessel_y_small_z_series<%1%>(%1%,%1%)", max_iter, pol);
   result *= prefix;

   if(!need_logs)
   {
      prefix = boost::math::tgamma(-v, pol) * boost::math::cos_pi(v, pol) * p / constants::pi<T>();
   }
   else
   {
      int sgn;
      prefix = boost::math::lgamma(-v, &sgn, pol) + p;
      prefix = exp(prefix) * sgn / constants::pi<T>();
   }
   bessel_y_small_z_series_term_b<T, Policy> s2(v, x);
   max_iter = policies::get_max_series_iterations<Policy>();

   T b = boost::math::tools::sum_series(s2, boost::math::policies::get_epsilon<T, Policy>(), max_iter);

   result -= scale * prefix * b;
   return result;
}

template <class T, class Policy>
T bessel_yn_small_z(int n, T z, T* scale, const Policy& pol)
{
   //
   // See http://functions.wolfram.com/Bessel-TypeFunctions/BesselY/06/01/04/01/02/
   //
   // Note that when called we assume that x < epsilon and n is a positive integer.
   //
   BOOST_MATH_STD_USING
   BOOST_MATH_ASSERT(n >= 0);
   BOOST_MATH_ASSERT((z < policies::get_epsilon<T, Policy>()));

   if(n == 0)
   {
      return (2 / constants::pi<T>()) * (log(z / 2) +  constants::euler<T>());
   }
   else if(n == 1)
   {
      return (z / constants::pi<T>()) * log(z / 2) 
         - 2 / (constants::pi<T>() * z) 
         - (z / (2 * constants::pi<T>())) * (1 - 2 * constants::euler<T>());
   }
   else if(n == 2)
   {
      return (z * z) / (4 * constants::pi<T>()) * log(z / 2) 
         - (4 / (constants::pi<T>() * z * z)) 
         - ((z * z) / (8 * constants::pi<T>())) * (T(3)/2 - 2 * constants::euler<T>());
   }
   else
   {
      T p = pow(z / 2, n);
      T result = -((boost::math::factorial<T>(n - 1, pol) / constants::pi<T>()));
      if(p * tools::max_value<T>() < result)
      {
         T div = tools::max_value<T>() / 8;
         result /= div;
         *scale /= div;
         if(p * tools::max_value<T>() < result)
         {
            return -policies::raise_overflow_error<T>("bessel_yn_small_z<%1%>(%1%,%1%)", 0, pol);
         }
      }
      return result / p;
   }
}

}}} // namespaces

#endif // BOOST_MATH_BESSEL_JN_SERIES_HPP

