//  Copyright (c) 2015 John Maddock
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef BOOST_MATH_ELLINT_RG_HPP
#define BOOST_MATH_ELLINT_RG_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/ellint_rd.hpp>
#include <boost/math/special_functions/ellint_rf.hpp>
#include <boost/math/special_functions/pow.hpp>

namespace boost { namespace math { namespace detail{

   template <typename T, typename Policy>
   T ellint_rg_imp(T x, T y, T z, const Policy& pol)
   {
      BOOST_MATH_STD_USING
      static const char* function = "boost::math::ellint_rf<%1%>(%1%,%1%,%1%)";

      if(x < 0 || y < 0 || z < 0)
      {
         return policies::raise_domain_error<T>(function,
            "domain error, all arguments must be non-negative, "
            "only sensible result is %1%.",
            std::numeric_limits<T>::quiet_NaN(), pol);
      }
      //
      // Function is symmetric in x, y and z, but we require
      // (x - z)(y - z) >= 0 to avoid cancellation error in the result
      // which implies (for example) x >= z >= y
      //
      using std::swap;
      if(x < y)
         swap(x, y);
      if(x < z)
         swap(x, z);
      if(y > z)
         swap(y, z);
      
      BOOST_MATH_ASSERT(x >= z);
      BOOST_MATH_ASSERT(z >= y);
      //
      // Special cases from http://dlmf.nist.gov/19.20#ii
      //
      if(x == z)
      {
         if(y == z)
         {
            // x = y = z
            // This also works for x = y = z = 0 presumably.
            return sqrt(x);
         }
         else if(y == 0)
         {
            // x = y, z = 0
            return constants::pi<T>() * sqrt(x) / 4;
         }
         else
         {
            // x = z, y != 0
            swap(x, y);
            return (x == 0) ? T(sqrt(z) / 2) : T((z * ellint_rc_imp(x, z, pol) + sqrt(x)) / 2);
         }
      }
      else if(y == z)
      {
         if(x == 0)
            return constants::pi<T>() * sqrt(y) / 4;
         else
            return (y == 0) ? T(sqrt(x) / 2) : T((y * ellint_rc_imp(x, y, pol) + sqrt(x)) / 2);
      }
      else if(y == 0)
      {
         swap(y, z);
         //
         // Special handling for common case, from
         // Numerical Computation of Real or Complex Elliptic Integrals, eq.46
         //
         T xn = sqrt(x);
         T yn = sqrt(y);
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
         return ((boost::math::pow<2>((x0 + y0) / 2) - sum) * RF) / 2;
      }
      return (z * ellint_rf_imp(x, y, z, pol)
         - (x - z) * (y - z) * ellint_rd_imp(x, y, z, pol) / 3
         + sqrt(x * y / z)) / 2;
   }

} // namespace detail

template <class T1, class T2, class T3, class Policy>
inline typename tools::promote_args<T1, T2, T3>::type 
   ellint_rg(T1 x, T2 y, T3 z, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2, T3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   return policies::checked_narrowing_cast<result_type, Policy>(
      detail::ellint_rg_imp(
         static_cast<value_type>(x),
         static_cast<value_type>(y),
         static_cast<value_type>(z), pol), "boost::math::ellint_rf<%1%>(%1%,%1%,%1%)");
}

template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type 
   ellint_rg(T1 x, T2 y, T3 z)
{
   return ellint_rg(x, y, z, policies::policy<>());
}

}} // namespaces

#endif // BOOST_MATH_ELLINT_RG_HPP

