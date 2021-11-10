//  Copyright (c) 2006 Xiaogang Zhang, 2015 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  History:
//  XZ wrote the original of this file as part of the Google
//  Summer of Code 2006.  JM modified it to fit into the
//  Boost.Math conceptual framework better, and to correctly
//  handle the p < 0 case.
//  Updated 2015 to use Carlson's latest methods.
//

#ifndef BOOST_MATH_ELLINT_RJ_HPP
#define BOOST_MATH_ELLINT_RJ_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/ellint_rc.hpp>
#include <boost/math/special_functions/ellint_rf.hpp>
#include <boost/math/special_functions/ellint_rd.hpp>

// Carlson's elliptic integral of the third kind
// R_J(x, y, z, p) = 1.5 * \int_{0}^{\infty} (t+p)^{-1} [(t+x)(t+y)(t+z)]^{-1/2} dt
// Carlson, Numerische Mathematik, vol 33, 1 (1979)

namespace boost { namespace math { namespace detail{

template <typename T, typename Policy>
T ellint_rc1p_imp(T y, const Policy& pol)
{
   using namespace boost::math;
   // Calculate RC(1, 1 + x)
   BOOST_MATH_STD_USING

  static const char* function = "boost::math::ellint_rc<%1%>(%1%,%1%)";

   if(y == -1)
   {
      return policies::raise_domain_error<T>(function,
         "Argument y must not be zero but got %1%", y, pol);
   }

   // for 1 + y < 0, the integral is singular, return Cauchy principal value
   T result;
   if(y < -1)
   {
      result = sqrt(1 / -y) * detail::ellint_rc_imp(T(-y), T(-1 - y), pol);
   }
   else if(y == 0)
   {
      result = 1;
   }
   else if(y > 0)
   {
      result = atan(sqrt(y)) / sqrt(y);
   }
   else
   {
      if(y > -0.5)
      {
         T arg = sqrt(-y);
         result = (boost::math::log1p(arg, pol) - boost::math::log1p(-arg, pol)) / (2 * sqrt(-y));
      }
      else
      {
         result = log((1 + sqrt(-y)) / sqrt(1 + y)) / sqrt(-y);
      }
   }
   return result;
}

template <typename T, typename Policy>
T ellint_rj_imp(T x, T y, T z, T p, const Policy& pol)
{
   BOOST_MATH_STD_USING

   static const char* function = "boost::math::ellint_rj<%1%>(%1%,%1%,%1%)";

   if(x < 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument x must be non-negative, but got x = %1%", x, pol);
   }
   if(y < 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument y must be non-negative, but got y = %1%", y, pol);
   }
   if(z < 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument z must be non-negative, but got z = %1%", z, pol);
   }
   if(p == 0)
   {
      return policies::raise_domain_error<T>(function,
         "Argument p must not be zero, but got p = %1%", p, pol);
   }
   if(x + y == 0 || y + z == 0 || z + x == 0)
   {
      return policies::raise_domain_error<T>(function,
         "At most one argument can be zero, "
         "only possible result is %1%.", std::numeric_limits<T>::quiet_NaN(), pol);
   }

   // for p < 0, the integral is singular, return Cauchy principal value
   if(p < 0)
   {
      //
      // We must ensure that x < y < z.
      // Since the integral is symmetrical in x, y and z
      // we can just permute the values:
      //
      if(x > y)
         std::swap(x, y);
      if(y > z)
         std::swap(y, z);
      if(x > y)
         std::swap(x, y);

      BOOST_MATH_ASSERT(x <= y);
      BOOST_MATH_ASSERT(y <= z);

      T q = -p;
      p = (z * (x + y + q) - x * y) / (z + q);

      BOOST_MATH_ASSERT(p >= 0);

      T value = (p - z) * ellint_rj_imp(x, y, z, p, pol);
      value -= 3 * ellint_rf_imp(x, y, z, pol);
      value += 3 * sqrt((x * y * z) / (x * y + p * q)) * ellint_rc_imp(T(x * y + p * q), T(p * q), pol);
      value /= (z + q);
      return value;
   }

   //
   // Special cases from http://dlmf.nist.gov/19.20#iii
   //
   if(x == y)
   {
      if(x == z)
      {
         if(x == p)
         {
            // All values equal:
            return 1 / (x * sqrt(x));
         }
         else
         {
            // x = y = z:
            return 3 * (ellint_rc_imp(x, p, pol) - 1 / sqrt(x)) / (x - p);
         }
      }
      else
      {
         // x = y only, permute so y = z:
         using std::swap;
         swap(x, z);
         if(y == p)
         {
            return ellint_rd_imp(x, y, y, pol);
         }
         else if((std::max)(y, p) / (std::min)(y, p) > 1.2)
         {
            return 3 * (ellint_rc_imp(x, y, pol) - ellint_rc_imp(x, p, pol)) / (p - y);
         }
         // Otherwise fall through to normal method, special case above will suffer too much cancellation...
      }
   }
   if(y == z)
   {
      if(y == p)
      {
         // y = z = p:
         return ellint_rd_imp(x, y, y, pol);
      }
      else if((std::max)(y, p) / (std::min)(y, p) > 1.2)
      {
         // y = z:
         return 3 * (ellint_rc_imp(x, y, pol) - ellint_rc_imp(x, p, pol)) / (p - y);
      }
      // Otherwise fall through to normal method, special case above will suffer too much cancellation...
   }
   if(z == p)
   {
      return ellint_rd_imp(x, y, z, pol);
   }

   T xn = x;
   T yn = y;
   T zn = z;
   T pn = p;
   T An = (x + y + z + 2 * p) / 5;
   T A0 = An;
   T delta = (p - x) * (p - y) * (p - z);
   T Q = pow(tools::epsilon<T>() / 5, -T(1) / 8) * (std::max)((std::max)(fabs(An - x), fabs(An - y)), (std::max)(fabs(An - z), fabs(An - p)));

   unsigned n;
   T lambda;
   T Dn;
   T En;
   T rx, ry, rz, rp;
   T fmn = 1; // 4^-n
   T RC_sum = 0;

   for(n = 0; n < policies::get_max_series_iterations<Policy>(); ++n)
   {
      rx = sqrt(xn);
      ry = sqrt(yn);
      rz = sqrt(zn);
      rp = sqrt(pn);
      Dn = (rp + rx) * (rp + ry) * (rp + rz);
      En = delta / Dn;
      En /= Dn;
      if((En < -0.5) && (En > -1.5))
      {
         //
         // Occasionally En ~ -1, we then have no means of calculating
         // RC(1, 1+En) without terrible cancellation error, so we
         // need to get to 1+En directly.  By substitution we have
         //
         // 1+E_0 = 1 + (p-x)*(p-y)*(p-z)/((sqrt(p) + sqrt(x))*(sqrt(p)+sqrt(y))*(sqrt(p)+sqrt(z)))^2
         //       = 2*sqrt(p)*(p+sqrt(x) * (sqrt(y)+sqrt(z)) + sqrt(y)*sqrt(z)) / ((sqrt(p) + sqrt(x))*(sqrt(p) + sqrt(y)*(sqrt(p)+sqrt(z))))
         //
         // And since this is just an application of the duplication formula for RJ, the same
         // expression works for 1+En if we use x,y,z,p_n etc.
         // This branch is taken only once or twice at the start of iteration,
         // after than En reverts to it's usual very small values.
         //
         T b = 2 * rp * (pn + rx * (ry + rz) + ry * rz) / Dn;
         RC_sum += fmn / Dn * detail::ellint_rc_imp(T(1), b, pol);
      }
      else
      {
         RC_sum += fmn / Dn * ellint_rc1p_imp(En, pol);
      }
      lambda = rx * ry + rx * rz + ry * rz;

      // From here on we move to n+1:
      An = (An + lambda) / 4;
      fmn /= 4;

      if(fmn * Q < An)
         break;

      xn = (xn + lambda) / 4;
      yn = (yn + lambda) / 4;
      zn = (zn + lambda) / 4;
      pn = (pn + lambda) / 4;
      delta /= 64;
   }

   T X = fmn * (A0 - x) / An;
   T Y = fmn * (A0 - y) / An;
   T Z = fmn * (A0 - z) / An;
   T P = (-X - Y - Z) / 2;
   T E2 = X * Y + X * Z + Y * Z - 3 * P * P;
   T E3 = X * Y * Z + 2 * E2 * P + 4 * P * P * P;
   T E4 = (2 * X * Y * Z + E2 * P + 3 * P * P * P) * P;
   T E5 = X * Y * Z * P * P;
   T result = fmn * pow(An, T(-3) / 2) *
      (1 - 3 * E2 / 14 + E3 / 6 + 9 * E2 * E2 / 88 - 3 * E4 / 22 - 9 * E2 * E3 / 52 + 3 * E5 / 26 - E2 * E2 * E2 / 16
      + 3 * E3 * E3 / 40 + 3 * E2 * E4 / 20 + 45 * E2 * E2 * E3 / 272 - 9 * (E3 * E4 + E2 * E5) / 68);

   result += 6 * RC_sum;
   return result;
}

} // namespace detail

template <class T1, class T2, class T3, class T4, class Policy>
inline typename tools::promote_args<T1, T2, T3, T4>::type 
   ellint_rj(T1 x, T2 y, T3 z, T4 p, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2, T3, T4>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(
      detail::ellint_rj_imp(
         static_cast<value_type>(x),
         static_cast<value_type>(y),
         static_cast<value_type>(z),
         static_cast<value_type>(p),
         pol), "boost::math::ellint_rj<%1%>(%1%,%1%,%1%,%1%)");
}

template <class T1, class T2, class T3, class T4>
inline typename tools::promote_args<T1, T2, T3, T4>::type 
   ellint_rj(T1 x, T2 y, T3 z, T4 p)
{
   return ellint_rj(x, y, z, p, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_ELLINT_RJ_HPP

