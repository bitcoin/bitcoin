///////////////////////////////////////////////////////////////////////////////
//  Copyright 2014 Anton Bikineev
//  Copyright 2014 Christopher Kormanyos
//  Copyright 2014 John Maddock
//  Copyright 2014 Paul Bristow
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_HYPERGEOMETRIC_0F1_HPP
#define BOOST_MATH_HYPERGEOMETRIC_0F1_HPP

#include <boost/math/policies/policy.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/detail/hypergeometric_series.hpp>
#include <boost/math/special_functions/detail/hypergeometric_0F1_bessel.hpp>

namespace boost { namespace math { namespace detail {


   template <class T>
   struct hypergeometric_0F1_cf
   {
      //
      // We start this continued fraction at b on index -1
      // and treat the -1 and 0 cases as special cases.
      // We do this to avoid adding the continued fraction result
      // to 1 so that we can accurately evaluate for small results
      // as well as large ones.  See http://functions.wolfram.com/07.17.10.0002.01
      //
      T b, z;
      int k;
      hypergeometric_0F1_cf(T b_, T z_) : b(b_), z(z_), k(-2) {}
      typedef std::pair<T, T> result_type;

      result_type operator()()
      {
         ++k;
         if (k <= 0)
            return std::make_pair(z / b, 1);
         return std::make_pair(-z / ((k + 1) * (b + k)), 1 + z / ((k + 1) * (b + k)));
      }
   };

   template <class T, class Policy>
   T hypergeometric_0F1_cf_imp(T b, T z, const Policy& pol, const char* function)
   {
      hypergeometric_0F1_cf<T> evaluator(b, z);
      std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
      T cf = tools::continued_fraction_b(evaluator, policies::get_epsilon<T, Policy>(), max_iter);
      policies::check_series_iterations<T>(function, max_iter, pol);
      return cf;
   }


   template <class T, class Policy>
   inline T hypergeometric_0F1_imp(const T& b, const T& z, const Policy& pol)
   {
      const char* function = "boost::math::hypergeometric_0f1<%1%,%1%>(%1%, %1%)";
      BOOST_MATH_STD_USING

         // some special cases
         if (z == 0)
            return T(1);

      if ((b <= 0) && (b == floor(b)))
         return policies::raise_pole_error<T>(
            function,
            "Evaluation of 0f1 with nonpositive integer b = %1%.", b, pol);

      if (z < -5 && b > -5)
      {
         // Series is alternating and divergent, need to do something else here,
         // Bessel function relation is much more accurate, unless |b| is similarly
         // large to |z|, otherwise the CF formula suffers from cancellation when
         // the result would be very small.
         if (fabs(z / b) > 4)
            return hypergeometric_0F1_bessel(b, z, pol);
         return hypergeometric_0F1_cf_imp(b, z, pol, function);
      }
      // evaluation through Taylor series looks
      // more precisious than Bessel relation:
      // detail::hypergeometric_0f1_bessel(b, z, pol);
      return detail::hypergeometric_0F1_generic_series(b, z, pol);
   }

} // namespace detail

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type hypergeometric_0F1(T1 b, T2 z, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
      typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, Policy>(
      detail::hypergeometric_0F1_imp<value_type>(
         static_cast<value_type>(b),
         static_cast<value_type>(z),
         forwarding_policy()),
      "boost::math::hypergeometric_0F1<%1%>(%1%,%1%)");
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type hypergeometric_0F1(T1 b, T2 z)
{
   return hypergeometric_0F1(b, z, policies::policy<>());
}


} } // namespace boost::math

#endif // BOOST_MATH_HYPERGEOMETRIC_HPP
