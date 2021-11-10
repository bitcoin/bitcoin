//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_FUNCTIONS_IGAMMA_INVERSE_HPP
#define BOOST_MATH_SPECIAL_FUNCTIONS_IGAMMA_INVERSE_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/tuple.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/policies/error_handling.hpp>

namespace boost{ namespace math{

namespace detail{

template <class T>
T find_inverse_s(T p, T q)
{
   //
   // Computation of the Incomplete Gamma Function Ratios and their Inverse
   // ARMIDO R. DIDONATO and ALFRED H. MORRIS, JR.
   // ACM Transactions on Mathematical Software, Vol. 12, No. 4,
   // December 1986, Pages 377-393.
   //
   // See equation 32.
   //
   BOOST_MATH_STD_USING
   T t;
   if(p < 0.5)
   {
      t = sqrt(-2 * log(p));
   }
   else
   {
      t = sqrt(-2 * log(q));
   }
   static const double a[4] = { 3.31125922108741, 11.6616720288968, 4.28342155967104, 0.213623493715853 };
   static const double b[5] = { 1, 6.61053765625462, 6.40691597760039, 1.27364489782223, 0.3611708101884203e-1 };
   T s = t - tools::evaluate_polynomial(a, t) / tools::evaluate_polynomial(b, t);
   if(p < 0.5)
      s = -s;
   return s;
}

template <class T>
T didonato_SN(T a, T x, unsigned N, T tolerance = 0)
{
   //
   // Computation of the Incomplete Gamma Function Ratios and their Inverse
   // ARMIDO R. DIDONATO and ALFRED H. MORRIS, JR.
   // ACM Transactions on Mathematical Software, Vol. 12, No. 4,
   // December 1986, Pages 377-393.
   //
   // See equation 34.
   //
   T sum = 1;
   if(N >= 1)
   {
      T partial = x / (a + 1);
      sum += partial;
      for(unsigned i = 2; i <= N; ++i)
      {
         partial *= x / (a + i);
         sum += partial;
         if(partial < tolerance)
            break;
      }
   }
   return sum;
}

template <class T, class Policy>
inline T didonato_FN(T p, T a, T x, unsigned N, T tolerance, const Policy& pol)
{
   //
   // Computation of the Incomplete Gamma Function Ratios and their Inverse
   // ARMIDO R. DIDONATO and ALFRED H. MORRIS, JR.
   // ACM Transactions on Mathematical Software, Vol. 12, No. 4,
   // December 1986, Pages 377-393.
   //
   // See equation 34.
   //
   BOOST_MATH_STD_USING
   T u = log(p) + boost::math::lgamma(a + 1, pol);
   return exp((u + x - log(didonato_SN(a, x, N, tolerance))) / a);
}

template <class T, class Policy>
T find_inverse_gamma(T a, T p, T q, const Policy& pol, bool* p_has_10_digits)
{
   //
   // In order to understand what's going on here, you will
   // need to refer to:
   //
   // Computation of the Incomplete Gamma Function Ratios and their Inverse
   // ARMIDO R. DIDONATO and ALFRED H. MORRIS, JR.
   // ACM Transactions on Mathematical Software, Vol. 12, No. 4,
   // December 1986, Pages 377-393.
   //
   BOOST_MATH_STD_USING

   T result;
   *p_has_10_digits = false;

   if(a == 1)
   {
      result = -log(q);
      BOOST_MATH_INSTRUMENT_VARIABLE(result);
   }
   else if(a < 1)
   {
      T g = boost::math::tgamma(a, pol);
      T b = q * g;
      BOOST_MATH_INSTRUMENT_VARIABLE(g);
      BOOST_MATH_INSTRUMENT_VARIABLE(b);
      if((b > 0.6) || ((b >= 0.45) && (a >= 0.3)))
      {
         // DiDonato & Morris Eq 21: 
         //
         // There is a slight variation from DiDonato and Morris here:
         // the first form given here is unstable when p is close to 1,
         // making it impossible to compute the inverse of Q(a,x) for small
         // q.  Fortunately the second form works perfectly well in this case.
         //
         T u;
         if((b * q > 1e-8) && (q > 1e-5))
         {
            u = pow(p * g * a, 1 / a);
            BOOST_MATH_INSTRUMENT_VARIABLE(u);
         }
         else
         {
            u = exp((-q / a) - constants::euler<T>());
            BOOST_MATH_INSTRUMENT_VARIABLE(u);
         }
         result = u / (1 - (u / (a + 1)));
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else if((a < 0.3) && (b >= 0.35))
      {
         // DiDonato & Morris Eq 22:
         T t = exp(-constants::euler<T>() - b);
         T u = t * exp(t);
         result = t * exp(u);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else if((b > 0.15) || (a >= 0.3))
      {
         // DiDonato & Morris Eq 23:
         T y = -log(b);
         T u = y - (1 - a) * log(y);
         result = y - (1 - a) * log(u) - log(1 + (1 - a) / (1 + u));
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else if (b > 0.1)
      {
         // DiDonato & Morris Eq 24:
         T y = -log(b);
         T u = y - (1 - a) * log(y);
         result = y - (1 - a) * log(u) - log((u * u + 2 * (3 - a) * u + (2 - a) * (3 - a)) / (u * u + (5 - a) * u + 2));
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else
      {
         // DiDonato & Morris Eq 25:
         T y = -log(b);
         T c1 = (a - 1) * log(y);
         T c1_2 = c1 * c1;
         T c1_3 = c1_2 * c1;
         T c1_4 = c1_2 * c1_2;
         T a_2 = a * a;
         T a_3 = a_2 * a;

         T c2 = (a - 1) * (1 + c1);
         T c3 = (a - 1) * (-(c1_2 / 2) + (a - 2) * c1 + (3 * a - 5) / 2);
         T c4 = (a - 1) * ((c1_3 / 3) - (3 * a - 5) * c1_2 / 2 + (a_2 - 6 * a + 7) * c1 + (11 * a_2 - 46 * a + 47) / 6);
         T c5 = (a - 1) * (-(c1_4 / 4)
                           + (11 * a - 17) * c1_3 / 6
                           + (-3 * a_2 + 13 * a -13) * c1_2
                           + (2 * a_3 - 25 * a_2 + 72 * a - 61) * c1 / 2
                           + (25 * a_3 - 195 * a_2 + 477 * a - 379) / 12);

         T y_2 = y * y;
         T y_3 = y_2 * y;
         T y_4 = y_2 * y_2;
         result = y + c1 + (c2 / y) + (c3 / y_2) + (c4 / y_3) + (c5 / y_4);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
         if(b < 1e-28f)
            *p_has_10_digits = true;
      }
   }
   else
   {
      // DiDonato and Morris Eq 31:
      T s = find_inverse_s(p, q);

      BOOST_MATH_INSTRUMENT_VARIABLE(s);

      T s_2 = s * s;
      T s_3 = s_2 * s;
      T s_4 = s_2 * s_2;
      T s_5 = s_4 * s;
      T ra = sqrt(a);

      BOOST_MATH_INSTRUMENT_VARIABLE(ra);

      T w = a + s * ra + (s * s -1) / 3;
      w += (s_3 - 7 * s) / (36 * ra);
      w -= (3 * s_4 + 7 * s_2 - 16) / (810 * a);
      w += (9 * s_5 + 256 * s_3 - 433 * s) / (38880 * a * ra);

      BOOST_MATH_INSTRUMENT_VARIABLE(w);

      if((a >= 500) && (fabs(1 - w / a) < 1e-6))
      {
         result = w;
         *p_has_10_digits = true;
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else if (p > 0.5)
      {
         if(w < 3 * a)
         {
            result = w;
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         else
         {
            T D = (std::max)(T(2), T(a * (a - 1)));
            T lg = boost::math::lgamma(a, pol);
            T lb = log(q) + lg;
            if(lb < -D * 2.3)
            {
               // DiDonato and Morris Eq 25:
               T y = -lb;
               T c1 = (a - 1) * log(y);
               T c1_2 = c1 * c1;
               T c1_3 = c1_2 * c1;
               T c1_4 = c1_2 * c1_2;
               T a_2 = a * a;
               T a_3 = a_2 * a;

               T c2 = (a - 1) * (1 + c1);
               T c3 = (a - 1) * (-(c1_2 / 2) + (a - 2) * c1 + (3 * a - 5) / 2);
               T c4 = (a - 1) * ((c1_3 / 3) - (3 * a - 5) * c1_2 / 2 + (a_2 - 6 * a + 7) * c1 + (11 * a_2 - 46 * a + 47) / 6);
               T c5 = (a - 1) * (-(c1_4 / 4)
                                 + (11 * a - 17) * c1_3 / 6
                                 + (-3 * a_2 + 13 * a -13) * c1_2
                                 + (2 * a_3 - 25 * a_2 + 72 * a - 61) * c1 / 2
                                 + (25 * a_3 - 195 * a_2 + 477 * a - 379) / 12);

               T y_2 = y * y;
               T y_3 = y_2 * y;
               T y_4 = y_2 * y_2;
               result = y + c1 + (c2 / y) + (c3 / y_2) + (c4 / y_3) + (c5 / y_4);
               BOOST_MATH_INSTRUMENT_VARIABLE(result);
            }
            else
            {
               // DiDonato and Morris Eq 33:
               T u = -lb + (a - 1) * log(w) - log(1 + (1 - a) / (1 + w));
               result = -lb + (a - 1) * log(u) - log(1 + (1 - a) / (1 + u));
               BOOST_MATH_INSTRUMENT_VARIABLE(result);
            }
         }
      }
      else
      {
         T z = w;
         T ap1 = a + 1;
         T ap2 = a + 2;
         if(w < 0.15f * ap1)
         {
            // DiDonato and Morris Eq 35:
            T v = log(p) + boost::math::lgamma(ap1, pol);
            z = exp((v + w) / a);
            s = boost::math::log1p(z / ap1 * (1 + z / ap2), pol);
            z = exp((v + z - s) / a);
            s = boost::math::log1p(z / ap1 * (1 + z / ap2), pol);
            z = exp((v + z - s) / a);
            s = boost::math::log1p(z / ap1 * (1 + z / ap2 * (1 + z / (a + 3))), pol);
            z = exp((v + z - s) / a);
            BOOST_MATH_INSTRUMENT_VARIABLE(z);
         }

         if((z <= 0.01 * ap1) || (z > 0.7 * ap1))
         {
            result = z;
            if(z <= 0.002 * ap1)
               *p_has_10_digits = true;
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         else
         {
            // DiDonato and Morris Eq 36:
            T ls = log(didonato_SN(a, z, 100, T(1e-4)));
            T v = log(p) + boost::math::lgamma(ap1, pol);
            z = exp((v + z - ls) / a);
            result = z * (1 - (a * log(z) - z - v + ls) / (a - z));

            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
      }
   }
   return result;
}

template <class T, class Policy>
struct gamma_p_inverse_func
{
   gamma_p_inverse_func(T a_, T p_, bool inv) : a(a_), p(p_), invert(inv)
   {
      //
      // If p is too near 1 then P(x) - p suffers from cancellation
      // errors causing our root-finding algorithms to "thrash", better
      // to invert in this case and calculate Q(x) - (1-p) instead.
      //
      // Of course if p is *very* close to 1, then the answer we get will
      // be inaccurate anyway (because there's not enough information in p)
      // but at least we will converge on the (inaccurate) answer quickly.
      //
      if(p > 0.9)
      {
         p = 1 - p;
         invert = !invert;
      }
   }

   boost::math::tuple<T, T, T> operator()(const T& x)const
   {
      BOOST_FPU_EXCEPTION_GUARD
      //
      // Calculate P(x) - p and the first two derivates, or if the invert
      // flag is set, then Q(x) - q and it's derivatives.
      //
      typedef typename policies::evaluation<T, Policy>::type value_type;
      // typedef typename lanczos::lanczos<T, Policy>::type evaluation_type;
      typedef typename policies::normalise<
         Policy, 
         policies::promote_float<false>, 
         policies::promote_double<false>, 
         policies::discrete_quantile<>,
         policies::assert_undefined<> >::type forwarding_policy;

      BOOST_MATH_STD_USING  // For ADL of std functions.

      T f, f1;
      value_type ft;
      f = static_cast<T>(boost::math::detail::gamma_incomplete_imp(
               static_cast<value_type>(a), 
               static_cast<value_type>(x), 
               true, invert,
               forwarding_policy(), &ft));
      f1 = static_cast<T>(ft);
      T f2;
      T div = (a - x - 1) / x;
      f2 = f1;
      if((fabs(div) > 1) && (tools::max_value<T>() / fabs(div) < f2))
      {
         // overflow:
         f2 = -tools::max_value<T>() / 2;
      }
      else
      {
         f2 *= div;
      }

      if(invert)
      {
         f1 = -f1;
         f2 = -f2;
      }

      return boost::math::make_tuple(static_cast<T>(f - p), f1, f2);
   }
private:
   T a, p;
   bool invert;
};

template <class T, class Policy>
T gamma_p_inv_imp(T a, T p, const Policy& pol)
{
   BOOST_MATH_STD_USING  // ADL of std functions.

   static const char* function = "boost::math::gamma_p_inv<%1%>(%1%, %1%)";

   BOOST_MATH_INSTRUMENT_VARIABLE(a);
   BOOST_MATH_INSTRUMENT_VARIABLE(p);

   if(a <= 0)
      return policies::raise_domain_error<T>(function, "Argument a in the incomplete gamma function inverse must be >= 0 (got a=%1%).", a, pol);
   if((p < 0) || (p > 1))
      return policies::raise_domain_error<T>(function, "Probability must be in the range [0,1] in the incomplete gamma function inverse (got p=%1%).", p, pol);
   if(p == 1)
      return policies::raise_overflow_error<T>(function, 0, Policy());
   if(p == 0)
      return 0;
   bool has_10_digits;
   T guess = detail::find_inverse_gamma<T>(a, p, 1 - p, pol, &has_10_digits);
   if((policies::digits<T, Policy>() <= 36) && has_10_digits)
      return guess;
   T lower = tools::min_value<T>();
   if(guess <= lower)
      guess = tools::min_value<T>();
   BOOST_MATH_INSTRUMENT_VARIABLE(guess);
   //
   // Work out how many digits to converge to, normally this is
   // 2/3 of the digits in T, but if the first derivative is very
   // large convergence is slow, so we'll bump it up to full 
   // precision to prevent premature termination of the root-finding routine.
   //
   unsigned digits = policies::digits<T, Policy>();
   if(digits < 30)
   {
      digits *= 2;
      digits /= 3;
   }
   else
   {
      digits /= 2;
      digits -= 1;
   }
   if((a < 0.125) && (fabs(gamma_p_derivative(a, guess, pol)) > 1 / sqrt(tools::epsilon<T>())))
      digits = policies::digits<T, Policy>() - 2;
   //
   // Go ahead and iterate:
   //
   std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
   guess = tools::halley_iterate(
      detail::gamma_p_inverse_func<T, Policy>(a, p, false),
      guess,
      lower,
      tools::max_value<T>(),
      digits,
      max_iter);
   policies::check_root_iterations<T>(function, max_iter, pol);
   BOOST_MATH_INSTRUMENT_VARIABLE(guess);
   if(guess == lower)
      guess = policies::raise_underflow_error<T>(function, "Expected result known to be non-zero, but is smaller than the smallest available number.", pol);
   return guess;
}

template <class T, class Policy>
T gamma_q_inv_imp(T a, T q, const Policy& pol)
{
   BOOST_MATH_STD_USING  // ADL of std functions.

   static const char* function = "boost::math::gamma_q_inv<%1%>(%1%, %1%)";

   if(a <= 0)
      return policies::raise_domain_error<T>(function, "Argument a in the incomplete gamma function inverse must be >= 0 (got a=%1%).", a, pol);
   if((q < 0) || (q > 1))
      return policies::raise_domain_error<T>(function, "Probability must be in the range [0,1] in the incomplete gamma function inverse (got q=%1%).", q, pol);
   if(q == 0)
      return policies::raise_overflow_error<T>(function, 0, Policy());
   if(q == 1)
      return 0;
   bool has_10_digits;
   T guess = detail::find_inverse_gamma<T>(a, 1 - q, q, pol, &has_10_digits);
   if((policies::digits<T, Policy>() <= 36) && has_10_digits)
      return guess;
   T lower = tools::min_value<T>();
   if(guess <= lower)
      guess = tools::min_value<T>();
   //
   // Work out how many digits to converge to, normally this is
   // 2/3 of the digits in T, but if the first derivative is very
   // large convergence is slow, so we'll bump it up to full 
   // precision to prevent premature termination of the root-finding routine.
   //
   unsigned digits = policies::digits<T, Policy>();
   if(digits < 30)
   {
      digits *= 2;
      digits /= 3;
   }
   else
   {
      digits /= 2;
      digits -= 1;
   }
   if((a < 0.125) && (fabs(gamma_p_derivative(a, guess, pol)) > 1 / sqrt(tools::epsilon<T>())))
      digits = policies::digits<T, Policy>();
   //
   // Go ahead and iterate:
   //
   std::uintmax_t max_iter = policies::get_max_root_iterations<Policy>();
   guess = tools::halley_iterate(
      detail::gamma_p_inverse_func<T, Policy>(a, q, true),
      guess,
      lower,
      tools::max_value<T>(),
      digits,
      max_iter);
   policies::check_root_iterations<T>(function, max_iter, pol);
   if(guess == lower)
      guess = policies::raise_underflow_error<T>(function, "Expected result known to be non-zero, but is smaller than the smallest available number.", pol);
   return guess;
}

} // namespace detail

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   gamma_p_inv(T1 a, T2 p, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   return detail::gamma_p_inv_imp(
      static_cast<result_type>(a),
      static_cast<result_type>(p), pol);
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   gamma_q_inv(T1 a, T2 p, const Policy& pol)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   return detail::gamma_q_inv_imp(
      static_cast<result_type>(a),
      static_cast<result_type>(p), pol);
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   gamma_p_inv(T1 a, T2 p)
{
   return gamma_p_inv(a, p, policies::policy<>());
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   gamma_q_inv(T1 a, T2 p)
{
   return gamma_q_inv(a, p, policies::policy<>());
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_SPECIAL_FUNCTIONS_IGAMMA_INVERSE_HPP



