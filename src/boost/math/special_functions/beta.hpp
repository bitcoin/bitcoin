//  (C) Copyright John Maddock 2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_BETA_HPP
#define BOOST_MATH_SPECIAL_BETA_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/special_functions/gamma.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/expm1.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/math/tools/assert.hpp>
#include <cmath>

namespace boost{ namespace math{

namespace detail{

//
// Implementation of Beta(a,b) using the Lanczos approximation:
//
template <class T, class Lanczos, class Policy>
T beta_imp(T a, T b, const Lanczos&, const Policy& pol)
{
   BOOST_MATH_STD_USING  // for ADL of std names

   if(a <= 0)
      return policies::raise_domain_error<T>("boost::math::beta<%1%>(%1%,%1%)", "The arguments to the beta function must be greater than zero (got a=%1%).", a, pol);
   if(b <= 0)
      return policies::raise_domain_error<T>("boost::math::beta<%1%>(%1%,%1%)", "The arguments to the beta function must be greater than zero (got b=%1%).", b, pol);

   T result;

   T prefix = 1;
   T c = a + b;

   // Special cases:
   if((c == a) && (b < tools::epsilon<T>()))
      return 1 / b;
   else if((c == b) && (a < tools::epsilon<T>()))
      return 1 / a;
   if(b == 1)
      return 1/a;
   else if(a == 1)
      return 1/b;
   else if(c < tools::epsilon<T>())
   {
      result = c / a;
      result /= b;
      return result;
   }

   /*
   //
   // This code appears to be no longer necessary: it was
   // used to offset errors introduced from the Lanczos
   // approximation, but the current Lanczos approximations
   // are sufficiently accurate for all z that we can ditch
   // this.  It remains in the file for future reference...
   //
   // If a or b are less than 1, shift to greater than 1:
   if(a < 1)
   {
      prefix *= c / a;
      c += 1;
      a += 1;
   }
   if(b < 1)
   {
      prefix *= c / b;
      c += 1;
      b += 1;
   }
   */

   if(a < b)
      std::swap(a, b);

   // Lanczos calculation:
   T agh = static_cast<T>(a + Lanczos::g() - 0.5f);
   T bgh = static_cast<T>(b + Lanczos::g() - 0.5f);
   T cgh = static_cast<T>(c + Lanczos::g() - 0.5f);
   result = Lanczos::lanczos_sum_expG_scaled(a) * (Lanczos::lanczos_sum_expG_scaled(b) / Lanczos::lanczos_sum_expG_scaled(c));
   T ambh = a - 0.5f - b;
   if((fabs(b * ambh) < (cgh * 100)) && (a > 100))
   {
      // Special case where the base of the power term is close to 1
      // compute (1+x)^y instead:
      result *= exp(ambh * boost::math::log1p(-b / cgh, pol));
   }
   else
   {
      result *= pow(agh / cgh, a - T(0.5) - b);
   }
   if(cgh > 1e10f)
      // this avoids possible overflow, but appears to be marginally less accurate:
      result *= pow((agh / cgh) * (bgh / cgh), b);
   else
      result *= pow((agh * bgh) / (cgh * cgh), b);
   result *= sqrt(boost::math::constants::e<T>() / bgh);

   // If a and b were originally less than 1 we need to scale the result:
   result *= prefix;

   return result;
} // template <class T, class Lanczos> beta_imp(T a, T b, const Lanczos&)

//
// Generic implementation of Beta(a,b) without Lanczos approximation support
// (Caution this is slow!!!):
//
template <class T, class Policy>
T beta_imp(T a, T b, const lanczos::undefined_lanczos& l, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if(a <= 0)
      return policies::raise_domain_error<T>("boost::math::beta<%1%>(%1%,%1%)", "The arguments to the beta function must be greater than zero (got a=%1%).", a, pol);
   if(b <= 0)
      return policies::raise_domain_error<T>("boost::math::beta<%1%>(%1%,%1%)", "The arguments to the beta function must be greater than zero (got b=%1%).", b, pol);

   const T c = a + b;

   // Special cases:
   if ((c == a) && (b < tools::epsilon<T>()))
      return 1 / b;
   else if ((c == b) && (a < tools::epsilon<T>()))
      return 1 / a;
   if (b == 1)
      return 1 / a;
   else if (a == 1)
      return 1 / b;
   else if (c < tools::epsilon<T>())
   {
      T result = c / a;
      result /= b;
      return result;
   }

   // Regular cases start here:
   const T min_sterling = minimum_argument_for_bernoulli_recursion<T>();

   long shift_a = 0;
   long shift_b = 0;

   if(a < min_sterling)
      shift_a = 1 + ltrunc(min_sterling - a);
   if(b < min_sterling)
      shift_b = 1 + ltrunc(min_sterling - b);
   long shift_c = shift_a + shift_b;

   if ((shift_a == 0) && (shift_b == 0))
   {
      return pow(a / c, a) * pow(b / c, b) * scaled_tgamma_no_lanczos(a, pol) * scaled_tgamma_no_lanczos(b, pol) / scaled_tgamma_no_lanczos(c, pol);
   }
   else if ((a < 1) && (b < 1))
   {
      return boost::math::tgamma(a, pol) * (boost::math::tgamma(b, pol) / boost::math::tgamma(c));
   }
   else if(a < 1)
      return boost::math::tgamma(a, pol) * boost::math::tgamma_delta_ratio(b, a, pol);
   else if(b < 1)
      return boost::math::tgamma(b, pol) * boost::math::tgamma_delta_ratio(a, b, pol);
   else
   {
      T result = beta_imp(T(a + shift_a), T(b + shift_b), l, pol);
      //
      // Recursion:
      //
      for (long i = 0; i < shift_c; ++i)
      {
         result *= c + i;
         if (i < shift_a)
            result /= a + i;
         if (i < shift_b)
            result /= b + i;
      }
      return result;
   }

} // template <class T>T beta_imp(T a, T b, const lanczos::undefined_lanczos& l)


//
// Compute the leading power terms in the incomplete Beta:
//
// (x^a)(y^b)/Beta(a,b) when normalised, and
// (x^a)(y^b) otherwise.
//
// Almost all of the error in the incomplete beta comes from this
// function: particularly when a and b are large. Computing large
// powers are *hard* though, and using logarithms just leads to
// horrendous cancellation errors.
//
template <class T, class Lanczos, class Policy>
T ibeta_power_terms(T a,
                        T b,
                        T x,
                        T y,
                        const Lanczos&,
                        bool normalised,
                        const Policy& pol,
                        T prefix = 1,
                        const char* function = "boost::math::ibeta<%1%>(%1%, %1%, %1%)")
{
   BOOST_MATH_STD_USING

   if(!normalised)
   {
      // can we do better here?
      return pow(x, a) * pow(y, b);
   }

   T result;

   T c = a + b;

   // combine power terms with Lanczos approximation:
   T agh = static_cast<T>(a + Lanczos::g() - 0.5f);
   T bgh = static_cast<T>(b + Lanczos::g() - 0.5f);
   T cgh = static_cast<T>(c + Lanczos::g() - 0.5f);
   result = Lanczos::lanczos_sum_expG_scaled(c) / (Lanczos::lanczos_sum_expG_scaled(a) * Lanczos::lanczos_sum_expG_scaled(b));
   result *= prefix;
   // combine with the leftover terms from the Lanczos approximation:
   result *= sqrt(bgh / boost::math::constants::e<T>());
   result *= sqrt(agh / cgh);

   // l1 and l2 are the base of the exponents minus one:
   T l1 = (x * b - y * agh) / agh;
   T l2 = (y * a - x * bgh) / bgh;
   if(((std::min)(fabs(l1), fabs(l2)) < 0.2))
   {
      // when the base of the exponent is very near 1 we get really
      // gross errors unless extra care is taken:
      if((l1 * l2 > 0) || ((std::min)(a, b) < 1))
      {
         //
         // This first branch handles the simple cases where either:
         //
         // * The two power terms both go in the same direction
         // (towards zero or towards infinity).  In this case if either
         // term overflows or underflows, then the product of the two must
         // do so also.
         // *Alternatively if one exponent is less than one, then we
         // can't productively use it to eliminate overflow or underflow
         // from the other term.  Problems with spurious overflow/underflow
         // can't be ruled out in this case, but it is *very* unlikely
         // since one of the power terms will evaluate to a number close to 1.
         //
         if(fabs(l1) < 0.1)
         {
            result *= exp(a * boost::math::log1p(l1, pol));
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         else
         {
            result *= pow((x * cgh) / agh, a);
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         if(fabs(l2) < 0.1)
         {
            result *= exp(b * boost::math::log1p(l2, pol));
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         else
         {
            result *= pow((y * cgh) / bgh, b);
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
      }
      else if((std::max)(fabs(l1), fabs(l2)) < 0.5)
      {
         //
         // Both exponents are near one and both the exponents are
         // greater than one and further these two
         // power terms tend in opposite directions (one towards zero,
         // the other towards infinity), so we have to combine the terms
         // to avoid any risk of overflow or underflow.
         //
         // We do this by moving one power term inside the other, we have:
         //
         //    (1 + l1)^a * (1 + l2)^b
         //  = ((1 + l1)*(1 + l2)^(b/a))^a
         //  = (1 + l1 + l3 + l1*l3)^a   ;  l3 = (1 + l2)^(b/a) - 1
         //                                    = exp((b/a) * log(1 + l2)) - 1
         //
         // The tricky bit is deciding which term to move inside :-)
         // By preference we move the larger term inside, so that the
         // size of the largest exponent is reduced.  However, that can
         // only be done as long as l3 (see above) is also small.
         //
         bool small_a = a < b;
         T ratio = b / a;
         if((small_a && (ratio * l2 < 0.1)) || (!small_a && (l1 / ratio > 0.1)))
         {
            T l3 = boost::math::expm1(ratio * boost::math::log1p(l2, pol), pol);
            l3 = l1 + l3 + l3 * l1;
            l3 = a * boost::math::log1p(l3, pol);
            result *= exp(l3);
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
         else
         {
            T l3 = boost::math::expm1(boost::math::log1p(l1, pol) / ratio, pol);
            l3 = l2 + l3 + l3 * l2;
            l3 = b * boost::math::log1p(l3, pol);
            result *= exp(l3);
            BOOST_MATH_INSTRUMENT_VARIABLE(result);
         }
      }
      else if(fabs(l1) < fabs(l2))
      {
         // First base near 1 only:
         T l = a * boost::math::log1p(l1, pol)
            + b * log((y * cgh) / bgh);
         if((l <= tools::log_min_value<T>()) || (l >= tools::log_max_value<T>()))
         {
            l += log(result);
            if(l >= tools::log_max_value<T>())
               return policies::raise_overflow_error<T>(function, 0, pol);
            result = exp(l);
         }
         else
            result *= exp(l);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else
      {
         // Second base near 1 only:
         T l = b * boost::math::log1p(l2, pol)
            + a * log((x * cgh) / agh);
         if((l <= tools::log_min_value<T>()) || (l >= tools::log_max_value<T>()))
         {
            l += log(result);
            if(l >= tools::log_max_value<T>())
               return policies::raise_overflow_error<T>(function, 0, pol);
            result = exp(l);
         }
         else
            result *= exp(l);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
   }
   else
   {
      // general case:
      T b1 = (x * cgh) / agh;
      T b2 = (y * cgh) / bgh;
      l1 = a * log(b1);
      l2 = b * log(b2);
      BOOST_MATH_INSTRUMENT_VARIABLE(b1);
      BOOST_MATH_INSTRUMENT_VARIABLE(b2);
      BOOST_MATH_INSTRUMENT_VARIABLE(l1);
      BOOST_MATH_INSTRUMENT_VARIABLE(l2);
      if((l1 >= tools::log_max_value<T>())
         || (l1 <= tools::log_min_value<T>())
         || (l2 >= tools::log_max_value<T>())
         || (l2 <= tools::log_min_value<T>())
         )
      {
         // Oops, under/overflow, sidestep if we can:
         if(a < b)
         {
            T p1 = pow(b2, b / a);
            T l3 = a * (log(b1) + log(p1));
            if((l3 < tools::log_max_value<T>())
               && (l3 > tools::log_min_value<T>()))
            {
               result *= pow(p1 * b1, a);
            }
            else
            {
               l2 += l1 + log(result);
               if(l2 >= tools::log_max_value<T>())
                  return policies::raise_overflow_error<T>(function, 0, pol);
               result = exp(l2);
            }
         }
         else
         {
            T p1 = pow(b1, a / b);
            T l3 = (log(p1) + log(b2)) * b;
            if((l3 < tools::log_max_value<T>())
               && (l3 > tools::log_min_value<T>()))
            {
               result *= pow(p1 * b2, b);
            }
            else
            {
               l2 += l1 + log(result);
               if(l2 >= tools::log_max_value<T>())
                  return policies::raise_overflow_error<T>(function, 0, pol);
               result = exp(l2);
            }
         }
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else
      {
         // finally the normal case:
         result *= pow(b1, a) * pow(b2, b);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
   }

   BOOST_MATH_INSTRUMENT_VARIABLE(result);

   return result;
}
//
// Compute the leading power terms in the incomplete Beta:
//
// (x^a)(y^b)/Beta(a,b) when normalised, and
// (x^a)(y^b) otherwise.
//
// Almost all of the error in the incomplete beta comes from this
// function: particularly when a and b are large. Computing large
// powers are *hard* though, and using logarithms just leads to
// horrendous cancellation errors.
//
// This version is generic, slow, and does not use the Lanczos approximation.
//
template <class T, class Policy>
T ibeta_power_terms(T a,
                        T b,
                        T x,
                        T y,
                        const boost::math::lanczos::undefined_lanczos& l,
                        bool normalised,
                        const Policy& pol,
                        T prefix = 1,
                        const char* = "boost::math::ibeta<%1%>(%1%, %1%, %1%)")
{
   BOOST_MATH_STD_USING

   if(!normalised)
   {
      return prefix * pow(x, a) * pow(y, b);
   }

   T c = a + b;

   const T min_sterling = minimum_argument_for_bernoulli_recursion<T>();

   long shift_a = 0;
   long shift_b = 0;

   if (a < min_sterling)
      shift_a = 1 + ltrunc(min_sterling - a);
   if (b < min_sterling)
      shift_b = 1 + ltrunc(min_sterling - b);

   if ((shift_a == 0) && (shift_b == 0))
   {
      T power1, power2;
      if (a < b)
      {
         power1 = pow((x * y * c * c) / (a * b), a);
         power2 = pow((y * c) / b, b - a);
      }
      else
      {
         power1 = pow((x * y * c * c) / (a * b), b);
         power2 = pow((x * c) / a, a - b);
      }
      if (!(boost::math::isnormal)(power1) || !(boost::math::isnormal)(power2))
      {
         // We have to use logs :(
         return prefix * exp(a * log(x * c / a) + b * log(y * c / b)) * scaled_tgamma_no_lanczos(c, pol) / (scaled_tgamma_no_lanczos(a, pol) * scaled_tgamma_no_lanczos(b, pol));
      }
      return prefix * power1 * power2 * scaled_tgamma_no_lanczos(c, pol) / (scaled_tgamma_no_lanczos(a, pol) * scaled_tgamma_no_lanczos(b, pol));
   }

   T power1 = pow(x, a);
   T power2 = pow(y, b);
   T bet = beta_imp(a, b, l, pol);

   if(!(boost::math::isnormal)(power1) || !(boost::math::isnormal)(power2) || !(boost::math::isnormal)(bet))
   {
      int shift_c = shift_a + shift_b;
      T result = ibeta_power_terms(T(a + shift_a), T(b + shift_b), x, y, l, normalised, pol, prefix);
      if ((boost::math::isnormal)(result))
      {
         for (int i = 0; i < shift_c; ++i)
         {
            result /= c + i;
               if (i < shift_a)
               {
                  result *= a + i;
                     result /= x;
               }
            if (i < shift_b)
            {
               result *= b + i;
               result /= y;
            }
         }
         return prefix * result;
      }
      else
      {
         T log_result = log(x) * a + log(y) * b + log(prefix);
         if ((boost::math::isnormal)(bet))
            log_result -= log(bet);
         else
            log_result += boost::math::lgamma(c, pol) - boost::math::lgamma(a, pol) - boost::math::lgamma(b, pol);
         return exp(log_result);
      }
   }
   return prefix * power1 * (power2 / bet);
}
//
// Series approximation to the incomplete beta:
//
template <class T>
struct ibeta_series_t
{
   typedef T result_type;
   ibeta_series_t(T a_, T b_, T x_, T mult) : result(mult), x(x_), apn(a_), poch(1-b_), n(1) {}
   T operator()()
   {
      T r = result / apn;
      apn += 1;
      result *= poch * x / n;
      ++n;
      poch += 1;
      return r;
   }
private:
   T result, x, apn, poch;
   int n;
};

template <class T, class Lanczos, class Policy>
T ibeta_series(T a, T b, T x, T s0, const Lanczos&, bool normalised, T* p_derivative, T y, const Policy& pol)
{
   BOOST_MATH_STD_USING

   T result;

   BOOST_MATH_ASSERT((p_derivative == 0) || normalised);

   if(normalised)
   {
      T c = a + b;

      // incomplete beta power term, combined with the Lanczos approximation:
      T agh = static_cast<T>(a + Lanczos::g() - 0.5f);
      T bgh = static_cast<T>(b + Lanczos::g() - 0.5f);
      T cgh = static_cast<T>(c + Lanczos::g() - 0.5f);
      result = Lanczos::lanczos_sum_expG_scaled(c) / (Lanczos::lanczos_sum_expG_scaled(a) * Lanczos::lanczos_sum_expG_scaled(b));

      T l1 = log(cgh / bgh) * (b - 0.5f);
      T l2 = log(x * cgh / agh) * a;
      //
      // Check for over/underflow in the power terms:
      //
      if((l1 > tools::log_min_value<T>())
         && (l1 < tools::log_max_value<T>())
         && (l2 > tools::log_min_value<T>())
         && (l2 < tools::log_max_value<T>()))
      {
         if(a * b < bgh * 10)
            result *= exp((b - 0.5f) * boost::math::log1p(a / bgh, pol));
         else
            result *= pow(cgh / bgh, b - 0.5f);
         result *= pow(x * cgh / agh, a);
         result *= sqrt(agh / boost::math::constants::e<T>());

         if(p_derivative)
         {
            *p_derivative = result * pow(y, b);
            BOOST_MATH_ASSERT(*p_derivative >= 0);
         }
      }
      else
      {
         //
         // Oh dear, we need logs, and this *will* cancel:
         //
         result = log(result) + l1 + l2 + (log(agh) - 1) / 2;
         if(p_derivative)
            *p_derivative = exp(result + b * log(y));
         result = exp(result);
      }
   }
   else
   {
      // Non-normalised, just compute the power:
      result = pow(x, a);
   }
   if(result < tools::min_value<T>())
      return s0; // Safeguard: series can't cope with denorms.
   ibeta_series_t<T> s(a, b, x, result);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter, s0);
   policies::check_series_iterations<T>("boost::math::ibeta<%1%>(%1%, %1%, %1%) in ibeta_series (with lanczos)", max_iter, pol);
   return result;
}
//
// Incomplete Beta series again, this time without Lanczos support:
//
template <class T, class Policy>
T ibeta_series(T a, T b, T x, T s0, const boost::math::lanczos::undefined_lanczos& l, bool normalised, T* p_derivative, T y, const Policy& pol)
{
   BOOST_MATH_STD_USING

   T result;
   BOOST_MATH_ASSERT((p_derivative == 0) || normalised);

   if(normalised)
   {
      const T min_sterling = minimum_argument_for_bernoulli_recursion<T>();

      long shift_a = 0;
      long shift_b = 0;

      if (a < min_sterling)
         shift_a = 1 + ltrunc(min_sterling - a);
      if (b < min_sterling)
         shift_b = 1 + ltrunc(min_sterling - b);

      T c = a + b;

      if ((shift_a == 0) && (shift_b == 0))
      {
         result = pow(x * c / a, a) * pow(c / b, b) * scaled_tgamma_no_lanczos(c, pol) / (scaled_tgamma_no_lanczos(a, pol) * scaled_tgamma_no_lanczos(b, pol));
      }
      else if ((a < 1) && (b > 1))
         result = pow(x, a) / (boost::math::tgamma(a, pol) * boost::math::tgamma_delta_ratio(b, a, pol));
      else
      {
         T power = pow(x, a);
         T bet = beta_imp(a, b, l, pol);
         if (!(boost::math::isnormal)(power) || !(boost::math::isnormal)(bet))
         {
            result = exp(a * log(x) + boost::math::lgamma(c, pol) - boost::math::lgamma(a, pol) - boost::math::lgamma(b, pol));
         }
         else
            result = power / bet;
      }
      if(p_derivative)
      {
         *p_derivative = result * pow(y, b);
         BOOST_MATH_ASSERT(*p_derivative >= 0);
      }
   }
   else
   {
      // Non-normalised, just compute the power:
      result = pow(x, a);
   }
   if(result < tools::min_value<T>())
      return s0; // Safeguard: series can't cope with denorms.
   ibeta_series_t<T> s(a, b, x, result);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter, s0);
   policies::check_series_iterations<T>("boost::math::ibeta<%1%>(%1%, %1%, %1%) in ibeta_series (without lanczos)", max_iter, pol);
   return result;
}

//
// Continued fraction for the incomplete beta:
//
template <class T>
struct ibeta_fraction2_t
{
   typedef std::pair<T, T> result_type;

   ibeta_fraction2_t(T a_, T b_, T x_, T y_) : a(a_), b(b_), x(x_), y(y_), m(0) {}

   result_type operator()()
   {
      T aN = (a + m - 1) * (a + b + m - 1) * m * (b - m) * x * x;
      T denom = (a + 2 * m - 1);
      aN /= denom * denom;

      T bN = static_cast<T>(m);
      bN += (m * (b - m) * x) / (a + 2*m - 1);
      bN += ((a + m) * (a * y - b * x + 1 + m *(2 - x))) / (a + 2*m + 1);

      ++m;

      return std::make_pair(aN, bN);
   }

private:
   T a, b, x, y;
   int m;
};
//
// Evaluate the incomplete beta via the continued fraction representation:
//
template <class T, class Policy>
inline T ibeta_fraction2(T a, T b, T x, T y, const Policy& pol, bool normalised, T* p_derivative)
{
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   BOOST_MATH_STD_USING
   T result = ibeta_power_terms(a, b, x, y, lanczos_type(), normalised, pol);
   if(p_derivative)
   {
      *p_derivative = result;
      BOOST_MATH_ASSERT(*p_derivative >= 0);
   }
   if(result == 0)
      return result;

   ibeta_fraction2_t<T> f(a, b, x, y);
   T fract = boost::math::tools::continued_fraction_b(f, boost::math::policies::get_epsilon<T, Policy>());
   BOOST_MATH_INSTRUMENT_VARIABLE(fract);
   BOOST_MATH_INSTRUMENT_VARIABLE(result);
   return result / fract;
}
//
// Computes the difference between ibeta(a,b,x) and ibeta(a+k,b,x):
//
template <class T, class Policy>
T ibeta_a_step(T a, T b, T x, T y, int k, const Policy& pol, bool normalised, T* p_derivative)
{
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;

   BOOST_MATH_INSTRUMENT_VARIABLE(k);

   T prefix = ibeta_power_terms(a, b, x, y, lanczos_type(), normalised, pol);
   if(p_derivative)
   {
      *p_derivative = prefix;
      BOOST_MATH_ASSERT(*p_derivative >= 0);
   }
   prefix /= a;
   if(prefix == 0)
      return prefix;
   T sum = 1;
   T term = 1;
   // series summation from 0 to k-1:
   for(int i = 0; i < k-1; ++i)
   {
      term *= (a+b+i) * x / (a+i+1);
      sum += term;
   }
   prefix *= sum;

   return prefix;
}
//
// This function is only needed for the non-regular incomplete beta,
// it computes the delta in:
// beta(a,b,x) = prefix + delta * beta(a+k,b,x)
// it is currently only called for small k.
//
template <class T>
inline T rising_factorial_ratio(T a, T b, int k)
{
   // calculate:
   // (a)(a+1)(a+2)...(a+k-1)
   // _______________________
   // (b)(b+1)(b+2)...(b+k-1)

   // This is only called with small k, for large k
   // it is grossly inefficient, do not use outside it's
   // intended purpose!!!
   BOOST_MATH_INSTRUMENT_VARIABLE(k);
   if(k == 0)
      return 1;
   T result = 1;
   for(int i = 0; i < k; ++i)
      result *= (a+i) / (b+i);
   return result;
}
//
// Routine for a > 15, b < 1
//
// Begin by figuring out how large our table of Pn's should be,
// quoted accuracies are "guesstimates" based on empirical observation.
// Note that the table size should never exceed the size of our
// tables of factorials.
//
template <class T>
struct Pn_size
{
   // This is likely to be enough for ~35-50 digit accuracy
   // but it's hard to quantify exactly:
   static constexpr unsigned value =
      ::boost::math::max_factorial<T>::value >= 100 ? 50
   : ::boost::math::max_factorial<T>::value >= ::boost::math::max_factorial<double>::value ? 30
   : ::boost::math::max_factorial<T>::value >= ::boost::math::max_factorial<float>::value ? 15 : 1;
   static_assert(::boost::math::max_factorial<T>::value >= ::boost::math::max_factorial<float>::value, "Type does not provide for 35-50 digits of accuracy.");
};
template <>
struct Pn_size<float>
{
   static constexpr unsigned value = 15; // ~8-15 digit accuracy
   static_assert(::boost::math::max_factorial<float>::value >= 30, "Type does not provide for 8-15 digits of accuracy.");
};
template <>
struct Pn_size<double>
{
   static constexpr unsigned value = 30; // 16-20 digit accuracy
   static_assert(::boost::math::max_factorial<double>::value >= 60, "Type does not provide for 16-20 digits of accuracy.");
};
template <>
struct Pn_size<long double>
{
   static constexpr unsigned value = 50; // ~35-50 digit accuracy
   static_assert(::boost::math::max_factorial<long double>::value >= 100, "Type does not provide for ~35-50 digits of accuracy");
};

template <class T, class Policy>
T beta_small_b_large_a_series(T a, T b, T x, T y, T s0, T mult, const Policy& pol, bool normalised)
{
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   BOOST_MATH_STD_USING
   //
   // This is DiDonato and Morris's BGRAT routine, see Eq's 9 through 9.6.
   //
   // Some values we'll need later, these are Eq 9.1:
   //
   T bm1 = b - 1;
   T t = a + bm1 / 2;
   T lx, u;
   if(y < 0.35)
      lx = boost::math::log1p(-y, pol);
   else
      lx = log(x);
   u = -t * lx;
   // and from from 9.2:
   T prefix;
   T h = regularised_gamma_prefix(b, u, pol, lanczos_type());
   if(h <= tools::min_value<T>())
      return s0;
   if(normalised)
   {
      prefix = h / boost::math::tgamma_delta_ratio(a, b, pol);
      prefix /= pow(t, b);
   }
   else
   {
      prefix = full_igamma_prefix(b, u, pol) / pow(t, b);
   }
   prefix *= mult;
   //
   // now we need the quantity Pn, unfortunately this is computed
   // recursively, and requires a full history of all the previous values
   // so no choice but to declare a big table and hope it's big enough...
   //
   T p[ ::boost::math::detail::Pn_size<T>::value ] = { 1 };  // see 9.3.
   //
   // Now an initial value for J, see 9.6:
   //
   T j = boost::math::gamma_q(b, u, pol) / h;
   //
   // Now we can start to pull things together and evaluate the sum in Eq 9:
   //
   T sum = s0 + prefix * j;  // Value at N = 0
   // some variables we'll need:
   unsigned tnp1 = 1; // 2*N+1
   T lx2 = lx / 2;
   lx2 *= lx2;
   T lxp = 1;
   T t4 = 4 * t * t;
   T b2n = b;

   for(unsigned n = 1; n < sizeof(p)/sizeof(p[0]); ++n)
   {
      /*
      // debugging code, enable this if you want to determine whether
      // the table of Pn's is large enough...
      //
      static int max_count = 2;
      if(n > max_count)
      {
         max_count = n;
         std::cerr << "Max iterations in BGRAT was " << n << std::endl;
      }
      */
      //
      // begin by evaluating the next Pn from Eq 9.4:
      //
      tnp1 += 2;
      p[n] = 0;
      T mbn = b - n;
      unsigned tmp1 = 3;
      for(unsigned m = 1; m < n; ++m)
      {
         mbn = m * b - n;
         p[n] += mbn * p[n-m] / boost::math::unchecked_factorial<T>(tmp1);
         tmp1 += 2;
      }
      p[n] /= n;
      p[n] += bm1 / boost::math::unchecked_factorial<T>(tnp1);
      //
      // Now we want Jn from Jn-1 using Eq 9.6:
      //
      j = (b2n * (b2n + 1) * j + (u + b2n + 1) * lxp) / t4;
      lxp *= lx2;
      b2n += 2;
      //
      // pull it together with Eq 9:
      //
      T r = prefix * p[n] * j;
      sum += r;
      if(r > 1)
      {
         if(fabs(r) < fabs(tools::epsilon<T>() * sum))
            break;
      }
      else
      {
         if(fabs(r / tools::epsilon<T>()) < fabs(sum))
            break;
      }
   }
   return sum;
} // template <class T, class Lanczos>T beta_small_b_large_a_series(T a, T b, T x, T y, T s0, T mult, const Lanczos& l, bool normalised)

//
// For integer arguments we can relate the incomplete beta to the
// complement of the binomial distribution cdf and use this finite sum.
//
template <class T, class Policy>
T binomial_ccdf(T n, T k, T x, T y, const Policy& pol)
{
   BOOST_MATH_STD_USING // ADL of std names

   T result = pow(x, n);

   if(result > tools::min_value<T>())
   {
      T term = result;
      for(unsigned i = itrunc(T(n - 1)); i > k; --i)
      {
         term *= ((i + 1) * y) / ((n - i) * x);
         result += term;
      }
   }
   else
   {
      // First term underflows so we need to start at the mode of the
      // distribution and work outwards:
      int start = itrunc(n * x);
      if(start <= k + 1)
         start = itrunc(k + 2);
      result = pow(x, start) * pow(y, n - start) * boost::math::binomial_coefficient<T>(itrunc(n), itrunc(start), pol);
      if(result == 0)
      {
         // OK, starting slightly above the mode didn't work,
         // we'll have to sum the terms the old fashioned way:
         for(unsigned i = start - 1; i > k; --i)
         {
            result += pow(x, (int)i) * pow(y, n - i) * boost::math::binomial_coefficient<T>(itrunc(n), itrunc(i), pol);
         }
      }
      else
      {
         T term = result;
         T start_term = result;
         for(unsigned i = start - 1; i > k; --i)
         {
            term *= ((i + 1) * y) / ((n - i) * x);
            result += term;
         }
         term = start_term;
         for(unsigned i = start + 1; i <= n; ++i)
         {
            term *= (n - i + 1) * x / (i * y);
            result += term;
         }
      }
   }

   return result;
}


//
// The incomplete beta function implementation:
// This is just a big bunch of spaghetti code to divide up the
// input range and select the right implementation method for
// each domain:
//
template <class T, class Policy>
T ibeta_imp(T a, T b, T x, const Policy& pol, bool inv, bool normalised, T* p_derivative)
{
   static const char* function = "boost::math::ibeta<%1%>(%1%, %1%, %1%)";
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   BOOST_MATH_STD_USING // for ADL of std math functions.

   BOOST_MATH_INSTRUMENT_VARIABLE(a);
   BOOST_MATH_INSTRUMENT_VARIABLE(b);
   BOOST_MATH_INSTRUMENT_VARIABLE(x);
   BOOST_MATH_INSTRUMENT_VARIABLE(inv);
   BOOST_MATH_INSTRUMENT_VARIABLE(normalised);

   bool invert = inv;
   T fract;
   T y = 1 - x;

   BOOST_MATH_ASSERT((p_derivative == 0) || normalised);

   if(p_derivative)
      *p_derivative = -1; // value not set.

   if((x < 0) || (x > 1))
      return policies::raise_domain_error<T>(function, "Parameter x outside the range [0,1] in the incomplete beta function (got x=%1%).", x, pol);

   if(normalised)
   {
      if(a < 0)
         return policies::raise_domain_error<T>(function, "The argument a to the incomplete beta function must be >= zero (got a=%1%).", a, pol);
      if(b < 0)
         return policies::raise_domain_error<T>(function, "The argument b to the incomplete beta function must be >= zero (got b=%1%).", b, pol);
      // extend to a few very special cases:
      if(a == 0)
      {
         if(b == 0)
            return policies::raise_domain_error<T>(function, "The arguments a and b to the incomplete beta function cannot both be zero, with x=%1%.", x, pol);
         if(b > 0)
            return static_cast<T>(inv ? 0 : 1);
      }
      else if(b == 0)
      {
         if(a > 0)
            return static_cast<T>(inv ? 1 : 0);
      }
   }
   else
   {
      if(a <= 0)
         return policies::raise_domain_error<T>(function, "The argument a to the incomplete beta function must be greater than zero (got a=%1%).", a, pol);
      if(b <= 0)
         return policies::raise_domain_error<T>(function, "The argument b to the incomplete beta function must be greater than zero (got b=%1%).", b, pol);
   }

   if(x == 0)
   {
      if(p_derivative)
      {
         *p_derivative = (a == 1) ? (T)1 : (a < 1) ? T(tools::max_value<T>() / 2) : T(tools::min_value<T>() * 2);
      }
      return (invert ? (normalised ? T(1) : boost::math::beta(a, b, pol)) : T(0));
   }
   if(x == 1)
   {
      if(p_derivative)
      {
         *p_derivative = (b == 1) ? T(1) : (b < 1) ? T(tools::max_value<T>() / 2) : T(tools::min_value<T>() * 2);
      }
      return (invert == 0 ? (normalised ? 1 : boost::math::beta(a, b, pol)) : 0);
   }
   if((a == 0.5f) && (b == 0.5f))
   {
      // We have an arcsine distribution:
      if(p_derivative)
      {
         *p_derivative = 1 / constants::pi<T>() * sqrt(y * x);
      }
      T p = invert ? asin(sqrt(y)) / constants::half_pi<T>() : asin(sqrt(x)) / constants::half_pi<T>();
      if(!normalised)
         p *= constants::pi<T>();
      return p;
   }
   if(a == 1)
   {
      std::swap(a, b);
      std::swap(x, y);
      invert = !invert;
   }
   if(b == 1)
   {
      //
      // Special case see: http://functions.wolfram.com/GammaBetaErf/BetaRegularized/03/01/01/
      //
      if(a == 1)
      {
         if(p_derivative)
            *p_derivative = 1;
         return invert ? y : x;
      }

      if(p_derivative)
      {
         *p_derivative = a * pow(x, a - 1);
      }
      T p;
      if(y < 0.5)
         p = invert ? T(-boost::math::expm1(a * boost::math::log1p(-y, pol), pol)) : T(exp(a * boost::math::log1p(-y, pol)));
      else
         p = invert ? T(-boost::math::powm1(x, a, pol)) : T(pow(x, a));
      if(!normalised)
         p /= a;
      return p;
   }

   if((std::min)(a, b) <= 1)
   {
      if(x > 0.5)
      {
         std::swap(a, b);
         std::swap(x, y);
         invert = !invert;
         BOOST_MATH_INSTRUMENT_VARIABLE(invert);
      }
      if((std::max)(a, b) <= 1)
      {
         // Both a,b < 1:
         if((a >= (std::min)(T(0.2), b)) || (pow(x, a) <= 0.9))
         {
            if(!invert)
            {
               fract = ibeta_series(a, b, x, T(0), lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
            else
            {
               fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
               invert = false;
               fract = -ibeta_series(a, b, x, fract, lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
         }
         else
         {
            std::swap(a, b);
            std::swap(x, y);
            invert = !invert;
            if(y >= 0.3)
            {
               if(!invert)
               {
                  fract = ibeta_series(a, b, x, T(0), lanczos_type(), normalised, p_derivative, y, pol);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
               else
               {
                  fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
                  invert = false;
                  fract = -ibeta_series(a, b, x, fract, lanczos_type(), normalised, p_derivative, y, pol);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
            }
            else
            {
               // Sidestep on a, and then use the series representation:
               T prefix;
               if(!normalised)
               {
                  prefix = rising_factorial_ratio(T(a+b), a, 20);
               }
               else
               {
                  prefix = 1;
               }
               fract = ibeta_a_step(a, b, x, y, 20, pol, normalised, p_derivative);
               if(!invert)
               {
                  fract = beta_small_b_large_a_series(T(a + 20), b, x, y, fract, prefix, pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
               else
               {
                  fract -= (normalised ? 1 : boost::math::beta(a, b, pol));
                  invert = false;
                  fract = -beta_small_b_large_a_series(T(a + 20), b, x, y, fract, prefix, pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
            }
         }
      }
      else
      {
         // One of a, b < 1 only:
         if((b <= 1) || ((x < 0.1) && (pow(b * x, a) <= 0.7)))
         {
            if(!invert)
            {
               fract = ibeta_series(a, b, x, T(0), lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
            else
            {
               fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
               invert = false;
               fract = -ibeta_series(a, b, x, fract, lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
         }
         else
         {
            std::swap(a, b);
            std::swap(x, y);
            invert = !invert;

            if(y >= 0.3)
            {
               if(!invert)
               {
                  fract = ibeta_series(a, b, x, T(0), lanczos_type(), normalised, p_derivative, y, pol);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
               else
               {
                  fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
                  invert = false;
                  fract = -ibeta_series(a, b, x, fract, lanczos_type(), normalised, p_derivative, y, pol);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
            }
            else if(a >= 15)
            {
               if(!invert)
               {
                  fract = beta_small_b_large_a_series(a, b, x, y, T(0), T(1), pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
               else
               {
                  fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
                  invert = false;
                  fract = -beta_small_b_large_a_series(a, b, x, y, fract, T(1), pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
            }
            else
            {
               // Sidestep to improve errors:
               T prefix;
               if(!normalised)
               {
                  prefix = rising_factorial_ratio(T(a+b), a, 20);
               }
               else
               {
                  prefix = 1;
               }
               fract = ibeta_a_step(a, b, x, y, 20, pol, normalised, p_derivative);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               if(!invert)
               {
                  fract = beta_small_b_large_a_series(T(a + 20), b, x, y, fract, prefix, pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
               else
               {
                  fract -= (normalised ? 1 : boost::math::beta(a, b, pol));
                  invert = false;
                  fract = -beta_small_b_large_a_series(T(a + 20), b, x, y, fract, prefix, pol, normalised);
                  BOOST_MATH_INSTRUMENT_VARIABLE(fract);
               }
            }
         }
      }
   }
   else
   {
      // Both a,b >= 1:
      T lambda;
      if(a < b)
      {
         lambda = a - (a + b) * x;
      }
      else
      {
         lambda = (a + b) * y - b;
      }
      if(lambda < 0)
      {
         std::swap(a, b);
         std::swap(x, y);
         invert = !invert;
         BOOST_MATH_INSTRUMENT_VARIABLE(invert);
      }

      if(b < 40)
      {
         if((floor(a) == a) && (floor(b) == b) && (a < static_cast<T>((std::numeric_limits<int>::max)() - 100)) && (y != 1))
         {
            // relate to the binomial distribution and use a finite sum:
            T k = a - 1;
            T n = b + k;
            fract = binomial_ccdf(n, k, x, y, pol);
            if(!normalised)
               fract *= boost::math::beta(a, b, pol);
            BOOST_MATH_INSTRUMENT_VARIABLE(fract);
         }
         else if(b * x <= 0.7)
         {
            if(!invert)
            {
               fract = ibeta_series(a, b, x, T(0), lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
            else
            {
               fract = -(normalised ? 1 : boost::math::beta(a, b, pol));
               invert = false;
               fract = -ibeta_series(a, b, x, fract, lanczos_type(), normalised, p_derivative, y, pol);
               BOOST_MATH_INSTRUMENT_VARIABLE(fract);
            }
         }
         else if(a > 15)
         {
            // sidestep so we can use the series representation:
            int n = itrunc(T(floor(b)), pol);
            if(n == b)
               --n;
            T bbar = b - n;
            T prefix;
            if(!normalised)
            {
               prefix = rising_factorial_ratio(T(a+bbar), bbar, n);
            }
            else
            {
               prefix = 1;
            }
            fract = ibeta_a_step(bbar, a, y, x, n, pol, normalised, static_cast<T*>(0));
            fract = beta_small_b_large_a_series(a,  bbar, x, y, fract, T(1), pol, normalised);
            fract /= prefix;
            BOOST_MATH_INSTRUMENT_VARIABLE(fract);
         }
         else if(normalised)
         {
            // The formula here for the non-normalised case is tricky to figure
            // out (for me!!), and requires two pochhammer calculations rather
            // than one, so leave it for now and only use this in the normalized case....
            int n = itrunc(T(floor(b)), pol);
            T bbar = b - n;
            if(bbar <= 0)
            {
               --n;
               bbar += 1;
            }
            fract = ibeta_a_step(bbar, a, y, x, n, pol, normalised, static_cast<T*>(0));
            fract += ibeta_a_step(a, bbar, x, y, 20, pol, normalised, static_cast<T*>(0));
            if(invert)
               fract -= 1;  // Note this line would need changing if we ever enable this branch in non-normalized case
            fract = beta_small_b_large_a_series(T(a+20),  bbar, x, y, fract, T(1), pol, normalised);
            if(invert)
            {
               fract = -fract;
               invert = false;
            }
            BOOST_MATH_INSTRUMENT_VARIABLE(fract);
         }
         else
         {
            fract = ibeta_fraction2(a, b, x, y, pol, normalised, p_derivative);
            BOOST_MATH_INSTRUMENT_VARIABLE(fract);
         }
      }
      else
      {
         fract = ibeta_fraction2(a, b, x, y, pol, normalised, p_derivative);
         BOOST_MATH_INSTRUMENT_VARIABLE(fract);
      }
   }
   if(p_derivative)
   {
      if(*p_derivative < 0)
      {
         *p_derivative = ibeta_power_terms(a, b, x, y, lanczos_type(), true, pol);
      }
      T div = y * x;

      if(*p_derivative != 0)
      {
         if((tools::max_value<T>() * div < *p_derivative))
         {
            // overflow, return an arbitrarily large value:
            *p_derivative = tools::max_value<T>() / 2;
         }
         else
         {
            *p_derivative /= div;
         }
      }
   }
   return invert ? (normalised ? 1 : boost::math::beta(a, b, pol)) - fract : fract;
} // template <class T, class Lanczos>T ibeta_imp(T a, T b, T x, const Lanczos& l, bool inv, bool normalised)

template <class T, class Policy>
inline T ibeta_imp(T a, T b, T x, const Policy& pol, bool inv, bool normalised)
{
   return ibeta_imp(a, b, x, pol, inv, normalised, static_cast<T*>(0));
}

template <class T, class Policy>
T ibeta_derivative_imp(T a, T b, T x, const Policy& pol)
{
   static const char* function = "ibeta_derivative<%1%>(%1%,%1%,%1%)";
   //
   // start with the usual error checks:
   //
   if(a <= 0)
      return policies::raise_domain_error<T>(function, "The argument a to the incomplete beta function must be greater than zero (got a=%1%).", a, pol);
   if(b <= 0)
      return policies::raise_domain_error<T>(function, "The argument b to the incomplete beta function must be greater than zero (got b=%1%).", b, pol);
   if((x < 0) || (x > 1))
      return policies::raise_domain_error<T>(function, "Parameter x outside the range [0,1] in the incomplete beta function (got x=%1%).", x, pol);
   //
   // Now the corner cases:
   //
   if(x == 0)
   {
      return (a > 1) ? 0 :
         (a == 1) ? 1 / boost::math::beta(a, b, pol) : policies::raise_overflow_error<T>(function, 0, pol);
   }
   else if(x == 1)
   {
      return (b > 1) ? 0 :
         (b == 1) ? 1 / boost::math::beta(a, b, pol) : policies::raise_overflow_error<T>(function, 0, pol);
   }
   //
   // Now the regular cases:
   //
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   T y = (1 - x) * x;
   T f1 = ibeta_power_terms<T>(a, b, x, 1 - x, lanczos_type(), true, pol, 1 / y, function);
   return f1;
}
//
// Some forwarding functions that dis-ambiguate the third argument type:
//
template <class RT1, class RT2, class Policy>
inline typename tools::promote_args<RT1, RT2>::type
   beta(RT1 a, RT2 b, const Policy&, const std::true_type*)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::beta_imp(static_cast<value_type>(a), static_cast<value_type>(b), evaluation_type(), forwarding_policy()), "boost::math::beta<%1%>(%1%,%1%)");
}
template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   beta(RT1 a, RT2 b, RT3 x, const std::false_type*)
{
   return boost::math::beta(a, b, x, policies::policy<>());
}
} // namespace detail

//
// The actual function entry-points now follow, these just figure out
// which Lanczos approximation to use
// and forward to the implementation functions:
//
template <class RT1, class RT2, class A>
inline typename tools::promote_args<RT1, RT2, A>::type
   beta(RT1 a, RT2 b, A arg)
{
   typedef typename policies::is_policy<A>::type tag;
   return boost::math::detail::beta(a, b, arg, static_cast<tag*>(0));
}

template <class RT1, class RT2>
inline typename tools::promote_args<RT1, RT2>::type
   beta(RT1 a, RT2 b)
{
   return boost::math::beta(a, b, policies::policy<>());
}

template <class RT1, class RT2, class RT3, class Policy>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   beta(RT1 a, RT2 b, RT3 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::ibeta_imp(static_cast<value_type>(a), static_cast<value_type>(b), static_cast<value_type>(x), forwarding_policy(), false, false), "boost::math::beta<%1%>(%1%,%1%,%1%)");
}

template <class RT1, class RT2, class RT3, class Policy>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   betac(RT1 a, RT2 b, RT3 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::ibeta_imp(static_cast<value_type>(a), static_cast<value_type>(b), static_cast<value_type>(x), forwarding_policy(), true, false), "boost::math::betac<%1%>(%1%,%1%,%1%)");
}
template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   betac(RT1 a, RT2 b, RT3 x)
{
   return boost::math::betac(a, b, x, policies::policy<>());
}

template <class RT1, class RT2, class RT3, class Policy>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibeta(RT1 a, RT2 b, RT3 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::ibeta_imp(static_cast<value_type>(a), static_cast<value_type>(b), static_cast<value_type>(x), forwarding_policy(), false, true), "boost::math::ibeta<%1%>(%1%,%1%,%1%)");
}
template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibeta(RT1 a, RT2 b, RT3 x)
{
   return boost::math::ibeta(a, b, x, policies::policy<>());
}

template <class RT1, class RT2, class RT3, class Policy>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibetac(RT1 a, RT2 b, RT3 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::ibeta_imp(static_cast<value_type>(a), static_cast<value_type>(b), static_cast<value_type>(x), forwarding_policy(), true, true), "boost::math::ibetac<%1%>(%1%,%1%,%1%)");
}
template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibetac(RT1 a, RT2 b, RT3 x)
{
   return boost::math::ibetac(a, b, x, policies::policy<>());
}

template <class RT1, class RT2, class RT3, class Policy>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibeta_derivative(RT1 a, RT2 b, RT3 x, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<RT1, RT2, RT3>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy,
      policies::promote_float<false>,
      policies::promote_double<false>,
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::ibeta_derivative_imp(static_cast<value_type>(a), static_cast<value_type>(b), static_cast<value_type>(x), forwarding_policy()), "boost::math::ibeta_derivative<%1%>(%1%,%1%,%1%)");
}
template <class RT1, class RT2, class RT3>
inline typename tools::promote_args<RT1, RT2, RT3>::type
   ibeta_derivative(RT1 a, RT2 b, RT3 x)
{
   return boost::math::ibeta_derivative(a, b, x, policies::policy<>());
}

} // namespace math
} // namespace boost

#include <boost/math/special_functions/detail/ibeta_inverse.hpp>
#include <boost/math/special_functions/detail/ibeta_inv_ab.hpp>

#endif // BOOST_MATH_SPECIAL_BETA_HPP
