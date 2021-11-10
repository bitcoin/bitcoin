//  Copyright (c) 2006 Xiaogang Zhang, 2015 John Maddock.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  History:
//  XZ wrote the original of this file as part of the Google
//  Summer of Code 2006.  JM modified it slightly to fit into the
//  Boost.Math conceptual framework better.
//  Updated 2015 to use Carlson's latest methods.

#ifndef BOOST_MATH_ELLINT_RD_HPP
#define BOOST_MATH_ELLINT_RD_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/ellint_rc.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/policies/error_handling.hpp>

// Carlson's elliptic integral of the second kind
// R_D(x, y, z) = R_J(x, y, z, z) = 1.5 * \int_{0}^{\infty} [(t+x)(t+y)]^{-1/2} (t+z)^{-3/2} dt
// Carlson, Numerische Mathematik, vol 33, 1 (1979)

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T ellint_rd_imp(T x, T y, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING
   using std::swap;

   static const char* function = "boost::math::ellint_rd<%1%>(%1%,%1%,%1%)";

   if(x < 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument x must be >= 0, but got %1%", x, pol);
   }
   if(y < 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument y must be >= 0, but got %1%", y, pol);
   }
   if(z <= 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument z must be > 0, but got %1%", z, pol);
   }
   if(x + y == 0)
   {
      return policies::raise_domain_error<T>(function,
         "At most one argument can be zero, but got, x + y = %1%", x + y, pol);
   }
   //
   // Special cases from http://dlmf.nist.gov/19.20#iv
   //
   using std::swap;
   if(x == z)
      swap(x, y);
   if(y == z)
   {
      if(x == y)
      {
         return 1 / (x * sqrt(x));
      }
      else if(x == 0)
      {
         return 3 * constants::pi<T>() / (4 * y * sqrt(y));
      }
      else
      {
         if((std::min)(x, y) / (std::max)(x, y) > 1.3)
            return 3 * (ellint_rc_imp(x, y, pol) - sqrt(x) / y) / (2 * (y - x));
         // Otherwise fall through to avoid cancellation in the above (RC(x,y) -> 1/x^0.5 as x -> y)
      }
   }
   if(x == y)
   {
      if((std::min)(x, z) / (std::max)(x, z) > 1.3)
         return 3 * (ellint_rc_imp(z, x, pol) - 1 / sqrt(z)) / (z - x);
      // Otherwise fall through to avoid cancellation in the above (RC(x,y) -> 1/x^0.5 as x -> y)
   }
   if(y == 0)
      swap(x, y);
   if(x == 0)
   {
      //
      // Special handling for common case, from
      // Numerical Computation of Real or Complex Elliptic Integrals, eq.47
      //
      T xn = sqrt(y);
      T yn = sqrt(z);
      T x0 = xn;
      T y0 = yn;
      T sum = 0;
      T sum_pow = 0.25f;

      while(fabs(xn - yn) >= 2.7 * tools::root_epsilon<T>() * fabs(xn))
      {
         T t = sqrt(xn * yn);
         xn = (xn + yn) / 2;
         yn = t;
         sum_pow *= 2;
         sum += sum_pow * boost::math::pow<2>(xn - yn);
      }
      T RF = constants::pi<T>() / (xn + yn);
      //
      // This following calculation suffers from serious cancellation when y ~ z
      // unless we combine terms.  We have:
      //
      // ( ((x0 + y0)/2)^2 - z ) / (z(y-z))
      //
      // Substituting y = x0^2 and z = y0^2 and simplifying we get the following:
      //
      T pt = (x0 + 3 * y0) / (4 * z * (x0 + y0));
      //
      // Since we've moved the denominator from eq.47 inside the expression, we
      // need to also scale "sum" by the same value:
      //
      pt -= sum / (z * (y - z));
      return pt * RF * 3;
   }

   T xn = x;
   T yn = y;
   T zn = z;
   T An = (x + y + 3 * z) / 5;
   T A0 = An;
   // This has an extra 1.2 fudge factor which is really only needed when x, y and z are close in magnitude:
   T Q = pow(tools::epsilon<T>() / 4, -T(1) / 8) * (std::max)((std::max)(An - x, An - y), An - z) * 1.2f;
   BOOST_MATH_INSTRUMENT_VARIABLE(Q);
   T lambda, rx, ry, rz;
   unsigned k = 0;
   T fn = 1;
   T RD_sum = 0;

   for(; k < policies::get_max_series_iterations<Policy>(); ++k)
   {
      rx = sqrt(xn);
      ry = sqrt(yn);
      rz = sqrt(zn);
      lambda = rx * ry + rx * rz + ry * rz;
      RD_sum += fn / (rz * (zn + lambda));
      An = (An + lambda) / 4;
      xn = (xn + lambda) / 4;
      yn = (yn + lambda) / 4;
      zn = (zn + lambda) / 4;
      fn /= 4;
      Q /= 4;
      BOOST_MATH_INSTRUMENT_VARIABLE(k);
      BOOST_MATH_INSTRUMENT_VARIABLE(RD_sum);
      BOOST_MATH_INSTRUMENT_VARIABLE(Q);
      if(Q < An)
         break;
   }

   policies::check_series_iterations<T, Policy>(function, k, pol);

   T X = fn * (A0 - x) / An;
   T Y = fn * (A0 - y) / An;
   T Z = -(X + Y) / 3;
   T E2 = X * Y - 6 * Z * Z;
   T E3 = (3 * X * Y - 8 * Z * Z) * Z;
   T E4 = 3 * (X * Y - Z * Z) * Z * Z;
   T E5 = X * Y * Z * Z * Z;

   T result = fn * pow(An, T(-3) / 2) *
      (1 - 3 * E2 / 14 + E3 / 6 + 9 * E2 * E2 / 88 - 3 * E4 / 22 - 9 * E2 * E3 / 52 + 3 * E5 / 26 - E2 * E2 * E2 / 16
      + 3 * E3 * E3 / 40 + 3 * E2 * E4 / 20 + 45 * E2 * E2 * E3 / 272 - 9 * (E3 * E4 + E2 * E5) / 68);
   BOOST_MATH_INSTRUMENT_VARIABLE(result);
   result += 3 * RD_sum;

   return result;
}

} // namespace detail

template <class T1, class T2, class T3, class Policy>
inline typename tools::promote_args<T1, T2, T3>::type 
   ellint_rd(T1 x, T2 y, T3 z, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2, T3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(
      detail::ellint_rd_imp(
         static_cast<value_type>(x),
         static_cast<value_type>(y),
         static_cast<value_type>(z), pol), "boost::math::ellint_rd<%1%>(%1%,%1%,%1%)");
}

template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type 
   ellint_rd(T1 x, T2 y, T3 z)
{
   return ellint_rd(x, y, z, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_ELLINT_RD_HPP

