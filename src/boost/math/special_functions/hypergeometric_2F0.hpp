///////////////////////////////////////////////////////////////////////////////
//  Copyright 2014 Anton Bikineev
//  Copyright 2014 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2014 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_HYPERGEOMETRIC_2F0_HPP
#define BOOST_MATH_HYPERGEOMETRIC_2F0_HPP

#include <boost/math/policies/policy.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/detail/hypergeometric_series.hpp>
#include <boost/math/special_functions/laguerre.hpp>
#include <boost/math/special_functions/hermite.hpp>
#include <boost/math/tools/fraction.hpp>

namespace boost { namespace math { namespace detail {

   template <class T>
   struct hypergeometric_2F0_cf
   {
      //
      // We start this continued fraction at b on index -1
      // and treat the -1 and 0 cases as special cases.
      // We do this to avoid adding the continued fraction result
      // to 1 so that we can accurately evaluate for small results
      // as well as large ones.  See  http://functions.wolfram.com/07.31.10.0002.01
      //
      T a1, a2, z;
      int k;
      hypergeometric_2F0_cf(T a1_, T a2_, T z_) : a1(a1_), a2(a2_), z(z_), k(-2) {}
      typedef std::pair<T, T> result_type;

      result_type operator()()
      {
         ++k;
         if (k <= 0)
            return std::make_pair(z * a1 * a2, 1);
         return std::make_pair(-z * (a1 + k) * (a2 + k) / (k + 1), 1 + z * (a1 + k) * (a2 + k) / (k + 1));
      }
   };

   template <class T, class Policy>
   T hypergeometric_2F0_cf_imp(T a1, T a2, T z, const Policy& pol, const char* function)
   {
      using namespace boost::math;
      hypergeometric_2F0_cf<T> evaluator(a1, a2, z);
      std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
      T cf = tools::continued_fraction_b(evaluator, policies::get_epsilon<T, Policy>(), max_iter);
      policies::check_series_iterations<T>(function, max_iter, pol);
      return cf;
   }


   template <class T, class Policy>
   inline T hypergeometric_2F0_imp(T a1, T a2, const T& z, const Policy& pol, bool asymptotic = false)
   {
      //
      // The terms in this series go to infinity unless one of a1 and a2 is a negative integer.
      //
      using std::swap;
      BOOST_MATH_STD_USING

      static const char* const function = "boost::math::hypergeometric_2F0<%1%,%1%,%1%>(%1%,%1%,%1%)";

      if (z == 0)
         return 1;

      bool is_a1_integer = (a1 == floor(a1));
      bool is_a2_integer = (a2 == floor(a2));

      if (!asymptotic && !is_a1_integer && !is_a2_integer)
         return boost::math::policies::raise_overflow_error<T>(function, 0, pol);
      if (!is_a1_integer || (a1 > 0))
      {
         swap(a1, a2);
         swap(is_a1_integer, is_a2_integer);
      }
      //
      // At this point a1 must be a negative integer:
      //
      if(!asymptotic && (!is_a1_integer || (a1 > 0)))
         return boost::math::policies::raise_overflow_error<T>(function, 0, pol);
      //
      // Special cases first:
      //
      if (a1 == 0)
         return 1;
      if ((a1 == a2 - 0.5f) && (z < 0))
      {
         // http://functions.wolfram.com/07.31.03.0083.01
         int n = static_cast<int>(static_cast<std::uintmax_t>(boost::math::lltrunc(-2 * a1)));
         T smz = sqrt(-z);
         return pow(2 / smz, -n) * boost::math::hermite(n, 1 / smz, pol);
      }

      if (is_a1_integer && is_a2_integer)
      {
         if ((a1 < 1) && (a2 <= a1))
         {
            const unsigned int n = static_cast<unsigned int>(static_cast<std::uintmax_t>(boost::math::lltrunc(-a1)));
            const unsigned int m = static_cast<unsigned int>(static_cast<std::uintmax_t>(boost::math::lltrunc(-a2 - n)));

            return (pow(z, T(n)) * boost::math::factorial<T>(n, pol)) *
               boost::math::laguerre(n, m, -(1 / z), pol);
         }
         else if ((a2 < 1) && (a1 <= a2))
         {
            // function is symmetric for a1 and a2
            const unsigned int n = static_cast<unsigned int>(static_cast<std::uintmax_t>(boost::math::lltrunc(-a2)));
            const unsigned int m = static_cast<unsigned int>(static_cast<std::uintmax_t>(boost::math::lltrunc(-a1 - n)));

            return (pow(z, T(n)) * boost::math::factorial<T>(n, pol)) *
               boost::math::laguerre(n, m, -(1 / z), pol);
         }
      }

      if ((a1 * a2 * z < 0) && (a2 < -5) && (fabs(a1 * a2 * z) > 0.5))
      {
         // Series is alternating and maybe divergent at least for the first few terms
         // (until a2 goes positive), try the continued fraction:
         return hypergeometric_2F0_cf_imp(a1, a2, z, pol, function);
      }

      return detail::hypergeometric_2F0_generic_series(a1, a2, z, pol);
   }

} // namespace detail

template <class T1, class T2, class T3, class Policy>
inline typename tools::promote_args<T1, T2, T3>::type hypergeometric_2F0(T1 a1, T2 a2, T3 z, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
      typedef typename tools::promote_args<T1, T2, T3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(
      detail::hypergeometric_2F0_imp<value_type>(
         static_cast<value_type>(a1),
         static_cast<value_type>(a2),
         static_cast<value_type>(z),
         forwarding_policy()),
      "boost::math::hypergeometric_2F0<%1%>(%1%,%1%,%1%)");
}

template <class T1, class T2, class T3>
inline typename tools::promote_args<T1, T2, T3>::type hypergeometric_2F0(T1 a1, T2 a2, T3 z)
{
   return hypergeometric_2F0(a1, a2, z, policies::policy<>());
}


  } } // namespace boost::math

#endif // BOOST_MATH_HYPERGEOMETRIC_HPP
