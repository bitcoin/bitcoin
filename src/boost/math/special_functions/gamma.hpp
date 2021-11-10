//  Copyright John Maddock 2006-7, 2013-20.
//  Copyright Paul A. Bristow 2007, 2013-14.
//  Copyright Nikhar Agrawal 2013-14
//  Copyright Christopher Kormanyos 2013-14, 2020

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SF_GAMMA_HPP
#define BOOST_MATH_SF_GAMMA_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/series.hpp>
#include <boost/math/tools/fraction.hpp>
#include <boost/math/tools/precision.hpp>
#include <boost/math/tools/promotion.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/config.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/special_functions/log1p.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/special_functions/powm1.hpp>
#include <boost/math/special_functions/sqrt1pm1.hpp>
#include <boost/math/special_functions/lanczos.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/detail/igamma_large.hpp>
#include <boost/math/special_functions/detail/unchecked_factorial.hpp>
#include <boost/math/special_functions/detail/lgamma_small.hpp>
#include <boost/math/special_functions/bernoulli.hpp>
#include <boost/math/special_functions/polygamma.hpp>

#include <cmath>
#include <algorithm>
#include <type_traits>

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable: 4702) // unreachable code (return after domain_error throw).
# pragma warning(disable: 4127) // conditional expression is constant.
# pragma warning(disable: 4100) // unreferenced formal parameter.
// Several variables made comments,
// but some difficulty as whether referenced on not may depend on macro values.
// So to be safe, 4100 warnings suppressed.
// TODO - revisit this?
#endif

namespace boost{ namespace math{

namespace detail{

template <class T>
inline bool is_odd(T v, const std::true_type&)
{
   int i = static_cast<int>(v);
   return i&1;
}
template <class T>
inline bool is_odd(T v, const std::false_type&)
{
   // Oh dear can't cast T to int!
   BOOST_MATH_STD_USING
   T modulus = v - 2 * floor(v/2);
   return static_cast<bool>(modulus != 0);
}
template <class T>
inline bool is_odd(T v)
{
   return is_odd(v, ::std::is_convertible<T, int>());
}

template <class T>
T sinpx(T z)
{
   // Ad hoc function calculates x * sin(pi * x),
   // taking extra care near when x is near a whole number.
   BOOST_MATH_STD_USING
   int sign = 1;
   if(z < 0)
   {
      z = -z;
   }
   T fl = floor(z);
   T dist;
   if(is_odd(fl))
   {
      fl += 1;
      dist = fl - z;
      sign = -sign;
   }
   else
   {
      dist = z - fl;
   }
   BOOST_MATH_ASSERT(fl >= 0);
   if(dist > 0.5)
      dist = 1 - dist;
   T result = sin(dist*boost::math::constants::pi<T>());
   return sign*z*result;
} // template <class T> T sinpx(T z)
//
// tgamma(z), with Lanczos support:
//
template <class T, class Policy, class Lanczos>
T gamma_imp(T z, const Policy& pol, const Lanczos& l)
{
   BOOST_MATH_STD_USING

   T result = 1;

#ifdef BOOST_MATH_INSTRUMENT
   static bool b = false;
   if(!b)
   {
      std::cout << "tgamma_imp called with " << typeid(z).name() << " " << typeid(l).name() << std::endl;
      b = true;
   }
#endif
   static const char* function = "boost::math::tgamma<%1%>(%1%)";

   if(z <= 0)
   {
      if(floor(z) == z)
         return policies::raise_pole_error<T>(function, "Evaluation of tgamma at a negative integer %1%.", z, pol);
      if(z <= -20)
      {
         result = gamma_imp(T(-z), pol, l) * sinpx(z);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
         if((fabs(result) < 1) && (tools::max_value<T>() * fabs(result) < boost::math::constants::pi<T>()))
            return -boost::math::sign(result) * policies::raise_overflow_error<T>(function, "Result of tgamma is too large to represent.", pol);
         result = -boost::math::constants::pi<T>() / result;
         if(result == 0)
            return policies::raise_underflow_error<T>(function, "Result of tgamma is too small to represent.", pol);
         if((boost::math::fpclassify)(result) == (int)FP_SUBNORMAL)
            return policies::raise_denorm_error<T>(function, "Result of tgamma is denormalized.", result, pol);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
         return result;
      }

      // shift z to > 1:
      while(z < 0)
      {
         result /= z;
         z += 1;
      }
   }
   BOOST_MATH_INSTRUMENT_VARIABLE(result);
   if((floor(z) == z) && (z < max_factorial<T>::value))
   {
      result *= unchecked_factorial<T>(itrunc(z, pol) - 1);
      BOOST_MATH_INSTRUMENT_VARIABLE(result);
   }
   else if (z < tools::root_epsilon<T>())
   {
      if (z < 1 / tools::max_value<T>())
         result = policies::raise_overflow_error<T>(function, 0, pol);
      result *= 1 / z - constants::euler<T>();
   }
   else
   {
      result *= Lanczos::lanczos_sum(z);
      T zgh = (z + static_cast<T>(Lanczos::g()) - boost::math::constants::half<T>());
      T lzgh = log(zgh);
      BOOST_MATH_INSTRUMENT_VARIABLE(result);
      BOOST_MATH_INSTRUMENT_VARIABLE(tools::log_max_value<T>());
      if(z * lzgh > tools::log_max_value<T>())
      {
         // we're going to overflow unless this is done with care:
         BOOST_MATH_INSTRUMENT_VARIABLE(zgh);
         if(lzgh * z / 2 > tools::log_max_value<T>())
            return boost::math::sign(result) * policies::raise_overflow_error<T>(function, "Result of tgamma is too large to represent.", pol);
         T hp = pow(zgh, (z / 2) - T(0.25));
         BOOST_MATH_INSTRUMENT_VARIABLE(hp);
         result *= hp / exp(zgh);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
         if(tools::max_value<T>() / hp < result)
            return boost::math::sign(result) * policies::raise_overflow_error<T>(function, "Result of tgamma is too large to represent.", pol);
         result *= hp;
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
      else
      {
         BOOST_MATH_INSTRUMENT_VARIABLE(zgh);
         BOOST_MATH_INSTRUMENT_VARIABLE(pow(zgh, z - boost::math::constants::half<T>()));
         BOOST_MATH_INSTRUMENT_VARIABLE(exp(zgh));
         result *= pow(zgh, z - boost::math::constants::half<T>()) / exp(zgh);
         BOOST_MATH_INSTRUMENT_VARIABLE(result);
      }
   }
   return result;
}
//
// lgamma(z) with Lanczos support:
//
template <class T, class Policy, class Lanczos>
T lgamma_imp(T z, const Policy& pol, const Lanczos& l, int* sign = 0)
{
#ifdef BOOST_MATH_INSTRUMENT
   static bool b = false;
   if(!b)
   {
      std::cout << "lgamma_imp called with " << typeid(z).name() << " " << typeid(l).name() << std::endl;
      b = true;
   }
#endif

   BOOST_MATH_STD_USING

   static const char* function = "boost::math::lgamma<%1%>(%1%)";

   T result = 0;
   int sresult = 1;
   if(z <= -tools::root_epsilon<T>())
   {
      // reflection formula:
      if(floor(z) == z)
         return policies::raise_pole_error<T>(function, "Evaluation of lgamma at a negative integer %1%.", z, pol);

      T t = sinpx(z);
      z = -z;
      if(t < 0)
      {
         t = -t;
      }
      else
      {
         sresult = -sresult;
      }
      result = log(boost::math::constants::pi<T>()) - lgamma_imp(z, pol, l) - log(t);
   }
   else if (z < tools::root_epsilon<T>())
   {
      if (0 == z)
         return policies::raise_pole_error<T>(function, "Evaluation of lgamma at %1%.", z, pol);
      if (4 * fabs(z) < tools::epsilon<T>())
         result = -log(fabs(z));
      else
         result = log(fabs(1 / z - constants::euler<T>()));
      if (z < 0)
         sresult = -1;
   }
   else if(z < 15)
   {
      typedef typename policies::precision<T, Policy>::type precision_type;
      typedef std::integral_constant<int,
         precision_type::value <= 0 ? 0 :
         precision_type::value <= 64 ? 64 :
         precision_type::value <= 113 ? 113 : 0
      > tag_type;

      result = lgamma_small_imp<T>(z, T(z - 1), T(z - 2), tag_type(), pol, l);
   }
   else if((z >= 3) && (z < 100) && (std::numeric_limits<T>::max_exponent >= 1024))
   {
      // taking the log of tgamma reduces the error, no danger of overflow here:
      result = log(gamma_imp(z, pol, l));
   }
   else
   {
      // regular evaluation:
      T zgh = static_cast<T>(z + Lanczos::g() - boost::math::constants::half<T>());
      result = log(zgh) - 1;
      result *= z - 0.5f;
      //
      // Only add on the lanczos sum part if we're going to need it:
      //
      if(result * tools::epsilon<T>() < 20)
         result += log(Lanczos::lanczos_sum_expG_scaled(z));
   }

   if(sign)
      *sign = sresult;
   return result;
}

//
// Incomplete gamma functions follow:
//
template <class T>
struct upper_incomplete_gamma_fract
{
private:
   T z, a;
   int k;
public:
   typedef std::pair<T,T> result_type;

   upper_incomplete_gamma_fract(T a1, T z1)
      : z(z1-a1+1), a(a1), k(0)
   {
   }

   result_type operator()()
   {
      ++k;
      z += 2;
      return result_type(k * (a - k), z);
   }
};

template <class T>
inline T upper_gamma_fraction(T a, T z, T eps)
{
   // Multiply result by z^a * e^-z to get the full
   // upper incomplete integral.  Divide by tgamma(z)
   // to normalise.
   upper_incomplete_gamma_fract<T> f(a, z);
   return 1 / (z - a + 1 + boost::math::tools::continued_fraction_a(f, eps));
}

template <class T>
struct lower_incomplete_gamma_series
{
private:
   T a, z, result;
public:
   typedef T result_type;
   lower_incomplete_gamma_series(T a1, T z1) : a(a1), z(z1), result(1){}

   T operator()()
   {
      T r = result;
      a += 1;
      result *= z/a;
      return r;
   }
};

template <class T, class Policy>
inline T lower_gamma_series(T a, T z, const Policy& pol, T init_value = 0)
{
   // Multiply result by ((z^a) * (e^-z) / a) to get the full
   // lower incomplete integral. Then divide by tgamma(a)
   // to get the normalised value.
   lower_incomplete_gamma_series<T> s(a, z);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>();
   T factor = policies::get_epsilon<T, Policy>();
   T result = boost::math::tools::sum_series(s, factor, max_iter, init_value);
   policies::check_series_iterations<T>("boost::math::detail::lower_gamma_series<%1%>(%1%)", max_iter, pol);
   return result;
}

//
// Fully generic tgamma and lgamma use Stirling's approximation
// with Bernoulli numbers.
//
template<class T>
std::size_t highest_bernoulli_index()
{
   const float digits10_of_type = (std::numeric_limits<T>::is_specialized
                                      ? static_cast<float>(std::numeric_limits<T>::digits10)
                                      : static_cast<float>(boost::math::tools::digits<T>() * 0.301F));

   // Find the high index n for Bn to produce the desired precision in Stirling's calculation.
   return static_cast<std::size_t>(18.0F + (0.6F * digits10_of_type));
}

template<class T>
int minimum_argument_for_bernoulli_recursion()
{
   BOOST_MATH_STD_USING

   const float digits10_of_type = (std::numeric_limits<T>::is_specialized
                                    ? (float) std::numeric_limits<T>::digits10
                                    : (float) (boost::math::tools::digits<T>() * 0.301F));

   int min_arg = (int) (digits10_of_type * 1.7F);

   if(digits10_of_type < 50.0F)
   {
      // The following code sequence has been modified
      // within the context of issue 396.

      // The calculation of the test-variable limit has now
      // been protected against overflow/underflow dangers.

      // The previous line looked like this and did, in fact,
      // underflow ldexp when using certain multiprecision types.

      // const float limit = std::ceil(std::pow(1.0f / std::ldexp(1.0f, 1-boost::math::tools::digits<T>()), 1.0f / 20.0f));

      // The new safe version of the limit check is now here.
      const float d2_minus_one = ((digits10_of_type / 0.301F) - 1.0F);
      const float limit        = ceil(exp((d2_minus_one * log(2.0F)) / 20.0F));

      min_arg = (int) ((std::min)(digits10_of_type * 1.7F, limit));
   }

   return min_arg;
}

template <class T, class Policy>
T scaled_tgamma_no_lanczos(const T& z, const Policy& pol, bool islog = false)
{
   BOOST_MATH_STD_USING
   //
   // Calculates tgamma(z) / (z/e)^z
   // Requires that our argument is large enough for Sterling's approximation to hold.
   // Used internally when combining gamma's of similar magnitude without logarithms.
   //
   BOOST_MATH_ASSERT(minimum_argument_for_bernoulli_recursion<T>() <= z);

   // Perform the Bernoulli series expansion of Stirling's approximation.

   const std::size_t number_of_bernoullis_b2n = policies::get_max_series_iterations<Policy>();

   T one_over_x_pow_two_n_minus_one = 1 / z;
   const T one_over_x2 = one_over_x_pow_two_n_minus_one * one_over_x_pow_two_n_minus_one;
   T sum = (boost::math::bernoulli_b2n<T>(1) / 2) * one_over_x_pow_two_n_minus_one;
   const T target_epsilon_to_break_loop = sum * boost::math::tools::epsilon<T>();
   const T half_ln_two_pi_over_z = sqrt(boost::math::constants::two_pi<T>() / z);
   T last_term = 2 * sum;

   for (std::size_t n = 2U;; ++n)
   {
      one_over_x_pow_two_n_minus_one *= one_over_x2;

      const std::size_t n2 = static_cast<std::size_t>(n * 2U);

      const T term = (boost::math::bernoulli_b2n<T>(static_cast<int>(n)) * one_over_x_pow_two_n_minus_one) / (n2 * (n2 - 1U));

      if ((n >= 3U) && (abs(term) < target_epsilon_to_break_loop))
      {
         // We have reached the desired precision in Stirling's expansion.
         // Adding additional terms to the sum of this divergent asymptotic
         // expansion will not improve the result.

         // Break from the loop.
         break;
      }
      if (n > number_of_bernoullis_b2n)
         return policies::raise_evaluation_error("scaled_tgamma_no_lanczos<%1%>()", "Exceeded maximum series iterations without reaching convergence, best approximation was %1%", T(exp(sum) * half_ln_two_pi_over_z), pol);

      sum += term;

      // Sanity check for divergence:
      T fterm = fabs(term);
      if(fterm > last_term)
         return policies::raise_evaluation_error("scaled_tgamma_no_lanczos<%1%>()", "Series became divergent without reaching convergence, best approximation was %1%", T(exp(sum) * half_ln_two_pi_over_z), pol);
      last_term = fterm;
   }

   // Complete Stirling's approximation.
   T scaled_gamma_value = islog ? T(sum + log(half_ln_two_pi_over_z)) : T(exp(sum) * half_ln_two_pi_over_z);
   return scaled_gamma_value;
}

// Forward declaration of the lgamma_imp template specialization.
template <class T, class Policy>
T lgamma_imp(T z, const Policy& pol, const lanczos::undefined_lanczos&, int* sign = 0);

template <class T, class Policy>
T gamma_imp(T z, const Policy& pol, const lanczos::undefined_lanczos&)
{
   BOOST_MATH_STD_USING

   static const char* function = "boost::math::tgamma<%1%>(%1%)";

   // Check if the argument of tgamma is identically zero.
   const bool is_at_zero = (z == 0);

   if((boost::math::isnan)(z) || (is_at_zero) || ((boost::math::isinf)(z) && (z < 0)))
      return policies::raise_domain_error<T>(function, "Evaluation of tgamma at %1%.", z, pol);

   const bool b_neg = (z < 0);

   const bool floor_of_z_is_equal_to_z = (floor(z) == z);

   // Special case handling of small factorials:
   if((!b_neg) && floor_of_z_is_equal_to_z && (z < boost::math::max_factorial<T>::value))
   {
      return boost::math::unchecked_factorial<T>(itrunc(z) - 1);
   }

   // Make a local, unsigned copy of the input argument.
   T zz((!b_neg) ? z : -z);

   // Special case for ultra-small z:
   if(zz < tools::cbrt_epsilon<T>())
   {
      const T a0(1);
      const T a1(boost::math::constants::euler<T>());
      const T six_euler_squared((boost::math::constants::euler<T>() * boost::math::constants::euler<T>()) * 6);
      const T a2((six_euler_squared -  boost::math::constants::pi_sqr<T>()) / 12);

      const T inverse_tgamma_series = z * ((a2 * z + a1) * z + a0);

      return 1 / inverse_tgamma_series;
   }

   // Scale the argument up for the calculation of lgamma,
   // and use downward recursion later for the final result.
   const int min_arg_for_recursion = minimum_argument_for_bernoulli_recursion<T>();

   int n_recur;

   if(zz < min_arg_for_recursion)
   {
      n_recur = boost::math::itrunc(min_arg_for_recursion - zz) + 1;

      zz += n_recur;
   }
   else
   {
      n_recur = 0;
   }
   if (!n_recur)
   {
      if (zz > tools::log_max_value<T>())
         return policies::raise_overflow_error<T>(function, 0, pol);
      if (log(zz) * zz / 2 > tools::log_max_value<T>())
         return policies::raise_overflow_error<T>(function, 0, pol);
   }
   T gamma_value = scaled_tgamma_no_lanczos(zz, pol);
   T power_term = pow(zz, zz / 2);
   T exp_term = exp(-zz);
   gamma_value *= (power_term * exp_term);
   if(!n_recur && (tools::max_value<T>() / power_term < gamma_value))
      return policies::raise_overflow_error<T>(function, 0, pol);
   gamma_value *= power_term;

   // Rescale the result using downward recursion if necessary.
   if(n_recur)
   {
      // The order of divides is important, if we keep subtracting 1 from zz
      // we DO NOT get back to z (cancellation error).  Further if z < epsilon
      // we would end up dividing by zero.  Also in order to prevent spurious
      // overflow with the first division, we must save dividing by |z| till last,
      // so the optimal order of divides is z+1, z+2, z+3...z+n_recur-1,z.
      zz = fabs(z) + 1;
      for(int k = 1; k < n_recur; ++k)
      {
         gamma_value /= zz;
         zz += 1;
      }
      gamma_value /= fabs(z);
   }

   // Return the result, accounting for possible negative arguments.
   if(b_neg)
   {
      // Provide special error analysis for:
      // * arguments in the neighborhood of a negative integer
      // * arguments exactly equal to a negative integer.

      // Check if the argument of tgamma is exactly equal to a negative integer.
      if(floor_of_z_is_equal_to_z)
         return policies::raise_pole_error<T>(function, "Evaluation of tgamma at a negative integer %1%.", z, pol);

      gamma_value *= sinpx(z);

      BOOST_MATH_INSTRUMENT_VARIABLE(gamma_value);

      const bool result_is_too_large_to_represent = (   (abs(gamma_value) < 1)
                                                     && ((tools::max_value<T>() * abs(gamma_value)) < boost::math::constants::pi<T>()));

      if(result_is_too_large_to_represent)
         return policies::raise_overflow_error<T>(function, "Result of tgamma is too large to represent.", pol);

      gamma_value = -boost::math::constants::pi<T>() / gamma_value;
      BOOST_MATH_INSTRUMENT_VARIABLE(gamma_value);

      if(gamma_value == 0)
         return policies::raise_underflow_error<T>(function, "Result of tgamma is too small to represent.", pol);

      if((boost::math::fpclassify)(gamma_value) == static_cast<int>(FP_SUBNORMAL))
         return policies::raise_denorm_error<T>(function, "Result of tgamma is denormalized.", gamma_value, pol);
   }

   return gamma_value;
}

template <class T, class Policy>
inline T log_gamma_near_1(const T& z, Policy const& pol)
{
   //
   // This is for the multiprecision case where there is
   // no lanczos support, use a taylor series at z = 1,
   // see https://www.wolframalpha.com/input/?i=taylor+series+lgamma(x)+at+x+%3D+1
   //
   BOOST_MATH_STD_USING // ADL of std names

   BOOST_MATH_ASSERT(fabs(z) < 1);

   T result = -constants::euler<T>() * z;

   T power_term = z * z / 2;
   int n = 2;
   T term = 0;

   do
   {
      term = power_term * boost::math::polygamma(n - 1, T(1), pol);
      result += term;
      ++n;
      power_term *= z / n;
   } while (fabs(result) * tools::epsilon<T>() < fabs(term));

   return result;
}

template <class T, class Policy>
T lgamma_imp(T z, const Policy& pol, const lanczos::undefined_lanczos&, int* sign)
{
   BOOST_MATH_STD_USING

   static const char* function = "boost::math::lgamma<%1%>(%1%)";

   // Check if the argument of lgamma is identically zero.
   const bool is_at_zero = (z == 0);

   if(is_at_zero)
      return policies::raise_domain_error<T>(function, "Evaluation of lgamma at zero %1%.", z, pol);
   if((boost::math::isnan)(z))
      return policies::raise_domain_error<T>(function, "Evaluation of lgamma at %1%.", z, pol);
   if((boost::math::isinf)(z))
      return policies::raise_overflow_error<T>(function, 0, pol);

   const bool b_neg = (z < 0);

   const bool floor_of_z_is_equal_to_z = (floor(z) == z);

   // Special case handling of small factorials:
   if((!b_neg) && floor_of_z_is_equal_to_z && (z < boost::math::max_factorial<T>::value))
   {
      if (sign)
         *sign = 1;
      return log(boost::math::unchecked_factorial<T>(itrunc(z) - 1));
   }

   // Make a local, unsigned copy of the input argument.
   T zz((!b_neg) ? z : -z);

   const int min_arg_for_recursion = minimum_argument_for_bernoulli_recursion<T>();

   T log_gamma_value;

   if (zz < min_arg_for_recursion)
   {
      // Here we simply take the logarithm of tgamma(). This is somewhat
      // inefficient, but simple. The rationale is that the argument here
      // is relatively small and overflow is not expected to be likely.
      if (sign)
         * sign = 1;
      if(fabs(z - 1) < 0.25)
      {
         log_gamma_value = log_gamma_near_1(T(zz - 1), pol);
      }
      else if(fabs(z - 2) < 0.25)
      {
         log_gamma_value = log_gamma_near_1(T(zz - 2), pol) + log(zz - 1);
      }
      else if (z > -tools::root_epsilon<T>())
      {
         // Reflection formula may fail if z is very close to zero, let the series
         // expansion for tgamma close to zero do the work:
         if (sign)
            *sign = z < 0 ? -1 : 1;
         return log(abs(gamma_imp(z, pol, lanczos::undefined_lanczos())));
      }
      else
      {
         // No issue with spurious overflow in reflection formula, 
         // just fall through to regular code:
         T g = gamma_imp(zz, pol, lanczos::undefined_lanczos());
         if (sign)
         {
            *sign = g < 0 ? -1 : 1;
         }
         log_gamma_value = log(abs(g));
      }
   }
   else
   {
      // Perform the Bernoulli series expansion of Stirling's approximation.
      T sum = scaled_tgamma_no_lanczos(zz, pol, true);
      log_gamma_value = zz * (log(zz) - 1) + sum;
   }

   int sign_of_result = 1;

   if(b_neg)
   {
      // Provide special error analysis if the argument is exactly
      // equal to a negative integer.

      // Check if the argument of lgamma is exactly equal to a negative integer.
      if(floor_of_z_is_equal_to_z)
         return policies::raise_pole_error<T>(function, "Evaluation of lgamma at a negative integer %1%.", z, pol);

      T t = sinpx(z);

      if(t < 0)
      {
         t = -t;
      }
      else
      {
         sign_of_result = -sign_of_result;
      }

      log_gamma_value = - log_gamma_value
                        + log(boost::math::constants::pi<T>())
                        - log(t);
   }

   if(sign != static_cast<int*>(0U)) { *sign = sign_of_result; }

   return log_gamma_value;
}

//
// This helper calculates tgamma(dz+1)-1 without cancellation errors,
// used by the upper incomplete gamma with z < 1:
//
template <class T, class Policy, class Lanczos>
T tgammap1m1_imp(T dz, Policy const& pol, const Lanczos& l)
{
   BOOST_MATH_STD_USING

   typedef typename policies::precision<T,Policy>::type precision_type;

   typedef std::integral_constant<int,
      precision_type::value <= 0 ? 0 :
      precision_type::value <= 64 ? 64 :
      precision_type::value <= 113 ? 113 : 0
   > tag_type;

   T result;
   if(dz < 0)
   {
      if(dz < -0.5)
      {
         // Best method is simply to subtract 1 from tgamma:
         result = boost::math::tgamma(1+dz, pol) - 1;
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
      else
      {
         // Use expm1 on lgamma:
         result = boost::math::expm1(-boost::math::log1p(dz, pol) 
            + lgamma_small_imp<T>(dz+2, dz + 1, dz, tag_type(), pol, l), pol);
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
   }
   else
   {
      if(dz < 2)
      {
         // Use expm1 on lgamma:
         result = boost::math::expm1(lgamma_small_imp<T>(dz+1, dz, dz-1, tag_type(), pol, l), pol);
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
      else
      {
         // Best method is simply to subtract 1 from tgamma:
         result = boost::math::tgamma(1+dz, pol) - 1;
         BOOST_MATH_INSTRUMENT_CODE(result);
      }
   }

   return result;
}

template <class T, class Policy>
inline T tgammap1m1_imp(T z, Policy const& pol,
                 const ::boost::math::lanczos::undefined_lanczos&)
{
   BOOST_MATH_STD_USING // ADL of std names

   if(fabs(z) < 0.55)
   {
      return boost::math::expm1(log_gamma_near_1(z, pol));
   }
   return boost::math::expm1(boost::math::lgamma(1 + z, pol));
}

//
// Series representation for upper fraction when z is small:
//
template <class T>
struct small_gamma2_series
{
   typedef T result_type;

   small_gamma2_series(T a_, T x_) : result(-x_), x(-x_), apn(a_+1), n(1){}

   T operator()()
   {
      T r = result / (apn);
      result *= x;
      result /= ++n;
      apn += 1;
      return r;
   }

private:
   T result, x, apn;
   int n;
};
//
// calculate power term prefix (z^a)(e^-z) used in the non-normalised
// incomplete gammas:
//
template <class T, class Policy>
T full_igamma_prefix(T a, T z, const Policy& pol)
{
   BOOST_MATH_STD_USING

   T prefix;
   if (z > tools::max_value<T>())
      return 0;
   T alz = a * log(z);

   if(z >= 1)
   {
      if((alz < tools::log_max_value<T>()) && (-z > tools::log_min_value<T>()))
      {
         prefix = pow(z, a) * exp(-z);
      }
      else if(a >= 1)
      {
         prefix = pow(z / exp(z/a), a);
      }
      else
      {
         prefix = exp(alz - z);
      }
   }
   else
   {
      if(alz > tools::log_min_value<T>())
      {
         prefix = pow(z, a) * exp(-z);
      }
      else if(z/a < tools::log_max_value<T>())
      {
         prefix = pow(z / exp(z/a), a);
      }
      else
      {
         prefix = exp(alz - z);
      }
   }
   //
   // This error handling isn't very good: it happens after the fact
   // rather than before it...
   //
   if((boost::math::fpclassify)(prefix) == (int)FP_INFINITE)
      return policies::raise_overflow_error<T>("boost::math::detail::full_igamma_prefix<%1%>(%1%, %1%)", "Result of incomplete gamma function is too large to represent.", pol);

   return prefix;
}
//
// Compute (z^a)(e^-z)/tgamma(a)
// most if the error occurs in this function:
//
template <class T, class Policy, class Lanczos>
T regularised_gamma_prefix(T a, T z, const Policy& pol, const Lanczos& l)
{
   BOOST_MATH_STD_USING
   if (z >= tools::max_value<T>())
      return 0;
   T agh = a + static_cast<T>(Lanczos::g()) - T(0.5);
   T prefix;
   T d = ((z - a) - static_cast<T>(Lanczos::g()) + T(0.5)) / agh;

   if(a < 1)
   {
      //
      // We have to treat a < 1 as a special case because our Lanczos
      // approximations are optimised against the factorials with a > 1,
      // and for high precision types especially (128-bit reals for example)
      // very small values of a can give rather erroneous results for gamma
      // unless we do this:
      //
      // TODO: is this still required?  Lanczos approx should be better now?
      //
      if(z <= tools::log_min_value<T>())
      {
         // Oh dear, have to use logs, should be free of cancellation errors though:
         return exp(a * log(z) - z - lgamma_imp(a, pol, l));
      }
      else
      {
         // direct calculation, no danger of overflow as gamma(a) < 1/a
         // for small a.
         return pow(z, a) * exp(-z) / gamma_imp(a, pol, l);
      }
   }
   else if((fabs(d*d*a) <= 100) && (a > 150))
   {
      // special case for large a and a ~ z.
      prefix = a * boost::math::log1pmx(d, pol) + z * static_cast<T>(0.5 - Lanczos::g()) / agh;
      prefix = exp(prefix);
   }
   else
   {
      //
      // general case.
      // direct computation is most accurate, but use various fallbacks
      // for different parts of the problem domain:
      //
      T alz = a * log(z / agh);
      T amz = a - z;
      if(((std::min)(alz, amz) <= tools::log_min_value<T>()) || ((std::max)(alz, amz) >= tools::log_max_value<T>()))
      {
         T amza = amz / a;
         if(((std::min)(alz, amz)/2 > tools::log_min_value<T>()) && ((std::max)(alz, amz)/2 < tools::log_max_value<T>()))
         {
            // compute square root of the result and then square it:
            T sq = pow(z / agh, a / 2) * exp(amz / 2);
            prefix = sq * sq;
         }
         else if(((std::min)(alz, amz)/4 > tools::log_min_value<T>()) && ((std::max)(alz, amz)/4 < tools::log_max_value<T>()) && (z > a))
         {
            // compute the 4th root of the result then square it twice:
            T sq = pow(z / agh, a / 4) * exp(amz / 4);
            prefix = sq * sq;
            prefix *= prefix;
         }
         else if((amza > tools::log_min_value<T>()) && (amza < tools::log_max_value<T>()))
         {
            prefix = pow((z * exp(amza)) / agh, a);
         }
         else
         {
            prefix = exp(alz + amz);
         }
      }
      else
      {
         prefix = pow(z / agh, a) * exp(amz);
      }
   }
   prefix *= sqrt(agh / boost::math::constants::e<T>()) / Lanczos::lanczos_sum_expG_scaled(a);
   return prefix;
}
//
// And again, without Lanczos support:
//
template <class T, class Policy>
T regularised_gamma_prefix(T a, T z, const Policy& pol, const lanczos::undefined_lanczos& l)
{
   BOOST_MATH_STD_USING

   if((a < 1) && (z < 1))
   {
      // No overflow possible since the power terms tend to unity as a,z -> 0
      return pow(z, a) * exp(-z) / boost::math::tgamma(a, pol);
   }
   else if(a > minimum_argument_for_bernoulli_recursion<T>())
   {
      T scaled_gamma = scaled_tgamma_no_lanczos(a, pol);
      T power_term = pow(z / a, a / 2);
      T a_minus_z = a - z;
      if ((0 == power_term) || (fabs(a_minus_z) > tools::log_max_value<T>()))
      {
         // The result is probably zero, but we need to be sure:
         return exp(a * log(z / a) + a_minus_z - log(scaled_gamma));
      }
      return (power_term * exp(a_minus_z)) * (power_term / scaled_gamma);
   }
   else
   {
      //
      // Usual case is to calculate the prefix at a+shift and recurse down
      // to the value we want:
      //
      const int min_z = minimum_argument_for_bernoulli_recursion<T>();
      long shift = 1 + ltrunc(min_z - a);
      T result = regularised_gamma_prefix(T(a + shift), z, pol, l);
      if (result != 0)
      {
         for (long i = 0; i < shift; ++i)
         {
            result /= z;
            result *= a + i;
         }
         return result;
      }
      else
      {
         // 
         // We failed, most probably we have z << 1, try again, this time
         // we calculate z^a e^-z / tgamma(a+shift), combining power terms
         // as we go.  And again recurse down to the result.
         //
         T scaled_gamma = scaled_tgamma_no_lanczos(T(a + shift), pol);
         T power_term_1 = pow(z / (a + shift), a);
         T power_term_2 = pow(a + shift, -shift);
         T power_term_3 = exp(a + shift - z);
         if ((0 == power_term_1) || (0 == power_term_2) || (0 == power_term_3) || (fabs(a + shift - z) > tools::log_max_value<T>()))
         {
            // We have no test case that gets here, most likely the type T
            // has a high precision but low exponent range:
            return exp(a * log(z) - z - boost::math::lgamma(a, pol));
         }
         result = power_term_1 * power_term_2 * power_term_3 / scaled_gamma;
         for (long i = 0; i < shift; ++i)
         {
            result *= a + i;
         }
         return result;
      }
   }
}
//
// Upper gamma fraction for very small a:
//
template <class T, class Policy>
inline T tgamma_small_upper_part(T a, T x, const Policy& pol, T* pgam = 0, bool invert = false, T* pderivative = 0)
{
   BOOST_MATH_STD_USING  // ADL of std functions.
   //
   // Compute the full upper fraction (Q) when a is very small:
   //
   T result;
   result = boost::math::tgamma1pm1(a, pol);
   if(pgam)
      *pgam = (result + 1) / a;
   T p = boost::math::powm1(x, a, pol);
   result -= p;
   result /= a;
   detail::small_gamma2_series<T> s(a, x);
   std::uintmax_t max_iter = policies::get_max_series_iterations<Policy>() - 10;
   p += 1;
   if(pderivative)
      *pderivative = p / (*pgam * exp(x));
   T init_value = invert ? *pgam : 0;
   result = -p * tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter, (init_value - result) / p);
   policies::check_series_iterations<T>("boost::math::tgamma_small_upper_part<%1%>(%1%, %1%)", max_iter, pol);
   if(invert)
      result = -result;
   return result;
}
//
// Upper gamma fraction for integer a:
//
template <class T, class Policy>
inline T finite_gamma_q(T a, T x, Policy const& pol, T* pderivative = 0)
{
   //
   // Calculates normalised Q when a is an integer:
   //
   BOOST_MATH_STD_USING
   T e = exp(-x);
   T sum = e;
   if(sum != 0)
   {
      T term = sum;
      for(unsigned n = 1; n < a; ++n)
      {
         term /= n;
         term *= x;
         sum += term;
      }
   }
   if(pderivative)
   {
      *pderivative = e * pow(x, a) / boost::math::unchecked_factorial<T>(itrunc(T(a - 1), pol));
   }
   return sum;
}
//
// Upper gamma fraction for half integer a:
//
template <class T, class Policy>
T finite_half_gamma_q(T a, T x, T* p_derivative, const Policy& pol)
{
   //
   // Calculates normalised Q when a is a half-integer:
   //
   BOOST_MATH_STD_USING
   T e = boost::math::erfc(sqrt(x), pol);
   if((e != 0) && (a > 1))
   {
      T term = exp(-x) / sqrt(constants::pi<T>() * x);
      term *= x;
      static const T half = T(1) / 2;
      term /= half;
      T sum = term;
      for(unsigned n = 2; n < a; ++n)
      {
         term /= n - half;
         term *= x;
         sum += term;
      }
      e += sum;
      if(p_derivative)
      {
         *p_derivative = 0;
      }
   }
   else if(p_derivative)
   {
      // We'll be dividing by x later, so calculate derivative * x:
      *p_derivative = sqrt(x) * exp(-x) / constants::root_pi<T>();
   }
   return e;
}
//
// Asymptotic approximation for large argument, see: https://dlmf.nist.gov/8.11#E2
//
template <class T>
struct incomplete_tgamma_large_x_series
{
   typedef T result_type;
   incomplete_tgamma_large_x_series(const T& a, const T& x)
      : a_poch(a - 1), z(x), term(1) {}
   T operator()()
   {
      T result = term;
      term *= a_poch / z;
      a_poch -= 1;
      return result;
   }
   T a_poch, z, term;
};

template <class T, class Policy>
T incomplete_tgamma_large_x(const T& a, const T& x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   incomplete_tgamma_large_x_series<T> s(a, x);
   std::uintmax_t max_iter = boost::math::policies::get_max_series_iterations<Policy>();
   T result = boost::math::tools::sum_series(s, boost::math::policies::get_epsilon<T, Policy>(), max_iter);
   boost::math::policies::check_series_iterations<T>("boost::math::tgamma<%1%>(%1%,%1%)", max_iter, pol);
   return result;
}


//
// Main incomplete gamma entry point, handles all four incomplete gamma's:
//
template <class T, class Policy>
T gamma_incomplete_imp(T a, T x, bool normalised, bool invert, 
                       const Policy& pol, T* p_derivative)
{
   static const char* function = "boost::math::gamma_p<%1%>(%1%, %1%)";
   if(a <= 0)
      return policies::raise_domain_error<T>(function, "Argument a to the incomplete gamma function must be greater than zero (got a=%1%).", a, pol);
   if(x < 0)
      return policies::raise_domain_error<T>(function, "Argument x to the incomplete gamma function must be >= 0 (got x=%1%).", x, pol);

   BOOST_MATH_STD_USING

   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;

   T result = 0; // Just to avoid warning C4701: potentially uninitialized local variable 'result' used

   if(a >= max_factorial<T>::value && !normalised)
   {
      //
      // When we're computing the non-normalized incomplete gamma
      // and a is large the result is rather hard to compute unless
      // we use logs.  There are really two options - if x is a long
      // way from a in value then we can reliably use methods 2 and 4
      // below in logarithmic form and go straight to the result.
      // Otherwise we let the regularized gamma take the strain
      // (the result is unlikely to underflow in the central region anyway)
      // and combine with lgamma in the hopes that we get a finite result.
      //
      if(invert && (a * 4 < x))
      {
         // This is method 4 below, done in logs:
         result = a * log(x) - x;
         if(p_derivative)
            *p_derivative = exp(result);
         result += log(upper_gamma_fraction(a, x, policies::get_epsilon<T, Policy>()));
      }
      else if(!invert && (a > 4 * x))
      {
         // This is method 2 below, done in logs:
         result = a * log(x) - x;
         if(p_derivative)
            *p_derivative = exp(result);
         T init_value = 0;
         result += log(detail::lower_gamma_series(a, x, pol, init_value) / a);
      }
      else
      {
         result = gamma_incomplete_imp(a, x, true, invert, pol, p_derivative);
         if(result == 0)
         {
            if(invert)
            {
               // Try http://functions.wolfram.com/06.06.06.0039.01
               result = 1 + 1 / (12 * a) + 1 / (288 * a * a);
               result = log(result) - a + (a - 0.5f) * log(a) + log(boost::math::constants::root_two_pi<T>());
               if(p_derivative)
                  *p_derivative = exp(a * log(x) - x);
            }
            else
            {
               // This is method 2 below, done in logs, we're really outside the
               // range of this method, but since the result is almost certainly
               // infinite, we should probably be OK:
               result = a * log(x) - x;
               if(p_derivative)
                  *p_derivative = exp(result);
               T init_value = 0;
               result += log(detail::lower_gamma_series(a, x, pol, init_value) / a);
            }
         }
         else
         {
            result = log(result) + boost::math::lgamma(a, pol);
         }
      }
      if(result > tools::log_max_value<T>())
         return policies::raise_overflow_error<T>(function, 0, pol);
      return exp(result);
   }

   BOOST_MATH_ASSERT((p_derivative == 0) || normalised);

   bool is_int, is_half_int;
   bool is_small_a = (a < 30) && (a <= x + 1) && (x < tools::log_max_value<T>());
   if(is_small_a)
   {
      T fa = floor(a);
      is_int = (fa == a);
      is_half_int = is_int ? false : (fabs(fa - a) == 0.5f);
   }
   else
   {
      is_int = is_half_int = false;
   }

   int eval_method;
   
   if(is_int && (x > 0.6))
   {
      // calculate Q via finite sum:
      invert = !invert;
      eval_method = 0;
   }
   else if(is_half_int && (x > 0.2))
   {
      // calculate Q via finite sum for half integer a:
      invert = !invert;
      eval_method = 1;
   }
   else if((x < tools::root_epsilon<T>()) && (a > 1))
   {
      eval_method = 6;
   }
   else if ((x > 1000) && ((a < x) || (fabs(a - 50) / x < 1)))
   {
      // calculate Q via asymptotic approximation:
      invert = !invert;
      eval_method = 7;
   }
   else if(x < 0.5)
   {
      //
      // Changeover criterion chosen to give a changeover at Q ~ 0.33
      //
      if(-0.4 / log(x) < a)
      {
         eval_method = 2;
      }
      else
      {
         eval_method = 3;
      }
   }
   else if(x < 1.1)
   {
      //
      // Changover here occurs when P ~ 0.75 or Q ~ 0.25:
      //
      if(x * 0.75f < a)
      {
         eval_method = 2;
      }
      else
      {
         eval_method = 3;
      }
   }
   else
   {
      //
      // Begin by testing whether we're in the "bad" zone
      // where the result will be near 0.5 and the usual
      // series and continued fractions are slow to converge:
      //
      bool use_temme = false;
      if(normalised && std::numeric_limits<T>::is_specialized && (a > 20))
      {
         T sigma = fabs((x-a)/a);
         if((a > 200) && (policies::digits<T, Policy>() <= 113))
         {
            //
            // This limit is chosen so that we use Temme's expansion
            // only if the result would be larger than about 10^-6.
            // Below that the regular series and continued fractions
            // converge OK, and if we use Temme's method we get increasing
            // errors from the dominant erfc term as it's (inexact) argument
            // increases in magnitude.
            //
            if(20 / a > sigma * sigma)
               use_temme = true;
         }
         else if(policies::digits<T, Policy>() <= 64)
         {
            // Note in this zone we can't use Temme's expansion for 
            // types longer than an 80-bit real:
            // it would require too many terms in the polynomials.
            if(sigma < 0.4)
               use_temme = true;
         }
      }
      if(use_temme)
      {
         eval_method = 5;
      }
      else
      {
         //
         // Regular case where the result will not be too close to 0.5.
         //
         // Changeover here occurs at P ~ Q ~ 0.5
         // Note that series computation of P is about x2 faster than continued fraction
         // calculation of Q, so try and use the CF only when really necessary, especially
         // for small x.
         //
         if(x - (1 / (3 * x)) < a)
         {
            eval_method = 2;
         }
         else
         {
            eval_method = 4;
            invert = !invert;
         }
      }
   }

   switch(eval_method)
   {
   case 0:
      {
         result = finite_gamma_q(a, x, pol, p_derivative);
         if(!normalised)
            result *= boost::math::tgamma(a, pol);
         break;
      }
   case 1:
      {
         result = finite_half_gamma_q(a, x, p_derivative, pol);
         if(!normalised)
            result *= boost::math::tgamma(a, pol);
         if(p_derivative && (*p_derivative == 0))
            *p_derivative = regularised_gamma_prefix(a, x, pol, lanczos_type());
         break;
      }
   case 2:
      {
         // Compute P:
         result = normalised ? regularised_gamma_prefix(a, x, pol, lanczos_type()) : full_igamma_prefix(a, x, pol);
         if(p_derivative)
            *p_derivative = result;
         if(result != 0)
         {
            //
            // If we're going to be inverting the result then we can
            // reduce the number of series evaluations by quite
            // a few iterations if we set an initial value for the
            // series sum based on what we'll end up subtracting it from
            // at the end.
            // Have to be careful though that this optimization doesn't 
            // lead to spurious numeric overflow.  Note that the
            // scary/expensive overflow checks below are more often
            // than not bypassed in practice for "sensible" input
            // values:
            //
            T init_value = 0;
            bool optimised_invert = false;
            if(invert)
            {
               init_value = (normalised ? 1 : boost::math::tgamma(a, pol));
               if(normalised || (result >= 1) || (tools::max_value<T>() * result > init_value))
               {
                  init_value /= result;
                  if(normalised || (a < 1) || (tools::max_value<T>() / a > init_value))
                  {
                     init_value *= -a;
                     optimised_invert = true;
                  }
                  else
                     init_value = 0;
               }
               else
                  init_value = 0;
            }
            result *= detail::lower_gamma_series(a, x, pol, init_value) / a;
            if(optimised_invert)
            {
               invert = false;
               result = -result;
            }
         }
         break;
      }
   case 3:
      {
         // Compute Q:
         invert = !invert;
         T g;
         result = tgamma_small_upper_part(a, x, pol, &g, invert, p_derivative);
         invert = false;
         if(normalised)
            result /= g;
         break;
      }
   case 4:
      {
         // Compute Q:
         result = normalised ? regularised_gamma_prefix(a, x, pol, lanczos_type()) : full_igamma_prefix(a, x, pol);
         if(p_derivative)
            *p_derivative = result;
         if(result != 0)
            result *= upper_gamma_fraction(a, x, policies::get_epsilon<T, Policy>());
         break;
      }
   case 5:
      {
         //
         // Use compile time dispatch to the appropriate
         // Temme asymptotic expansion.  This may be dead code
         // if T does not have numeric limits support, or has
         // too many digits for the most precise version of
         // these expansions, in that case we'll be calling
         // an empty function.
         //
         typedef typename policies::precision<T, Policy>::type precision_type;

         typedef std::integral_constant<int,
            precision_type::value <= 0 ? 0 :
            precision_type::value <= 53 ? 53 :
            precision_type::value <= 64 ? 64 :
            precision_type::value <= 113 ? 113 : 0
         > tag_type;

         result = igamma_temme_large(a, x, pol, static_cast<tag_type const*>(0));
         if(x >= a)
            invert = !invert;
         if(p_derivative)
            *p_derivative = regularised_gamma_prefix(a, x, pol, lanczos_type());
         break;
      }
   case 6:
      {
         // x is so small that P is necessarily very small too,
         // use http://functions.wolfram.com/GammaBetaErf/GammaRegularized/06/01/05/01/01/
         if(!normalised)
            result = pow(x, a) / (a);
         else
         {
            try 
            {
               result = pow(x, a) / boost::math::tgamma(a + 1, pol);
            }
            catch (const std::overflow_error&)
            {
               result = 0;
            }
         }
         result *= 1 - a * x / (a + 1);
         if (p_derivative)
            *p_derivative = regularised_gamma_prefix(a, x, pol, lanczos_type());
         break;
      }
   case 7:
   {
      // x is large,
      // Compute Q:
      result = normalised ? regularised_gamma_prefix(a, x, pol, lanczos_type()) : full_igamma_prefix(a, x, pol);
      if (p_derivative)
         *p_derivative = result;
      result /= x;
      if (result != 0)
         result *= incomplete_tgamma_large_x(a, x, pol);
      break;
   }
   }

   if(normalised && (result > 1))
      result = 1;
   if(invert)
   {
      T gam = normalised ? 1 : boost::math::tgamma(a, pol);
      result = gam - result;
   }
   if(p_derivative)
   {
      //
      // Need to convert prefix term to derivative:
      //
      if((x < 1) && (tools::max_value<T>() * x < *p_derivative))
      {
         // overflow, just return an arbitrarily large value:
         *p_derivative = tools::max_value<T>() / 2;
      }

      *p_derivative /= x;
   }

   return result;
}

//
// Ratios of two gamma functions:
//
template <class T, class Policy, class Lanczos>
T tgamma_delta_ratio_imp_lanczos(T z, T delta, const Policy& pol, const Lanczos& l)
{
   BOOST_MATH_STD_USING
   if(z < tools::epsilon<T>())
   {
      //
      // We get spurious numeric overflow unless we're very careful, this
      // can occur either inside Lanczos::lanczos_sum(z) or in the
      // final combination of terms, to avoid this, split the product up
      // into 2 (or 3) parts:
      //
      // G(z) / G(L) = 1 / (z * G(L)) ; z < eps, L = z + delta = delta
      //    z * G(L) = z * G(lim) * (G(L)/G(lim)) ; lim = largest factorial
      //
      if(boost::math::max_factorial<T>::value < delta)
      {
         T ratio = tgamma_delta_ratio_imp_lanczos(delta, T(boost::math::max_factorial<T>::value - delta), pol, l);
         ratio *= z;
         ratio *= boost::math::unchecked_factorial<T>(boost::math::max_factorial<T>::value - 1);
         return 1 / ratio;
      }
      else
      {
         return 1 / (z * boost::math::tgamma(z + delta, pol));
      }
   }
   T zgh = static_cast<T>(z + Lanczos::g() - constants::half<T>());
   T result;
   if(z + delta == z)
   {
      if (fabs(delta / zgh) < boost::math::tools::epsilon<T>())
      {
         // We have:
         // result = exp((constants::half<T>() - z) * boost::math::log1p(delta / zgh, pol));
         // 0.5 - z == -z
         // log1p(delta / zgh) = delta / zgh = delta / z
         // multiplying we get -delta.
         result = exp(-delta);
      }
      else
         // from the pow formula below... but this may actually be wrong, we just can't really calculate it :(
         result = 1;
   }
   else
   {
      if(fabs(delta) < 10)
      {
         result = exp((constants::half<T>() - z) * boost::math::log1p(delta / zgh, pol));
      }
      else
      {
         result = pow(zgh / (zgh + delta), z - constants::half<T>());
      }
      // Split the calculation up to avoid spurious overflow:
      result *= Lanczos::lanczos_sum(z) / Lanczos::lanczos_sum(T(z + delta));
   }
   result *= pow(constants::e<T>() / (zgh + delta), delta);
   return result;
}
//
// And again without Lanczos support this time:
//
template <class T, class Policy>
T tgamma_delta_ratio_imp_lanczos(T z, T delta, const Policy& pol, const lanczos::undefined_lanczos& l)
{
   BOOST_MATH_STD_USING

   //
   // We adjust z and delta so that both z and z+delta are large enough for
   // Sterling's approximation to hold.  We can then calculate the ratio
   // for the adjusted values, and rescale back down to z and z+delta.
   //
   // Get the required shifts first:
   //
   long numerator_shift = 0;
   long denominator_shift = 0;
   const int min_z = minimum_argument_for_bernoulli_recursion<T>();
   
   if (min_z > z)
      numerator_shift = 1 + ltrunc(min_z - z);
   if (min_z > z + delta)
      denominator_shift = 1 + ltrunc(min_z - z - delta);
   //
   // If the shifts are zero, then we can just combine scaled tgamma's
   // and combine the remaining terms:
   //
   if (numerator_shift == 0 && denominator_shift == 0)
   {
      T scaled_tgamma_num = scaled_tgamma_no_lanczos(z, pol);
      T scaled_tgamma_denom = scaled_tgamma_no_lanczos(T(z + delta), pol);
      T result = scaled_tgamma_num / scaled_tgamma_denom;
      result *= exp(z * boost::math::log1p(-delta / (z + delta), pol)) * pow((delta + z) / constants::e<T>(), -delta);
      return result;
   }
   //
   // We're going to have to rescale first, get the adjusted z and delta values,
   // plus the ratio for the adjusted values:
   //
   T zz = z + numerator_shift;
   T dd = delta - (numerator_shift - denominator_shift);
   T ratio = tgamma_delta_ratio_imp_lanczos(zz, dd, pol, l);
   //
   // Use gamma recurrence relations to get back to the original
   // z and z+delta:
   //
   for (long long i = 0; i < numerator_shift; ++i)
   {
      ratio /= (z + i);
      if (i < denominator_shift)
         ratio *= (z + delta + i);
   }
   for (long long i = numerator_shift; i < denominator_shift; ++i)
   {
      ratio *= (z + delta + i);
   }
   return ratio;
}

template <class T, class Policy>
T tgamma_delta_ratio_imp(T z, T delta, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if((z <= 0) || (z + delta <= 0))
   {
      // This isn't very sophisticated, or accurate, but it does work:
      return boost::math::tgamma(z, pol) / boost::math::tgamma(z + delta, pol);
   }

   if(floor(delta) == delta)
   {
      if(floor(z) == z)
      {
         //
         // Both z and delta are integers, see if we can just use table lookup
         // of the factorials to get the result:
         //
         if((z <= max_factorial<T>::value) && (z + delta <= max_factorial<T>::value))
         {
            return unchecked_factorial<T>((unsigned)itrunc(z, pol) - 1) / unchecked_factorial<T>((unsigned)itrunc(T(z + delta), pol) - 1);
         }
      }
      if(fabs(delta) < 20)
      {
         //
         // delta is a small integer, we can use a finite product:
         //
         if(delta == 0)
            return 1;
         if(delta < 0)
         {
            z -= 1;
            T result = z;
            while(0 != (delta += 1))
            {
               z -= 1;
               result *= z;
            }
            return result;
         }
         else
         {
            T result = 1 / z;
            while(0 != (delta -= 1))
            {
               z += 1;
               result /= z;
            }
            return result;
         }
      }
   }
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   return tgamma_delta_ratio_imp_lanczos(z, delta, pol, lanczos_type());
}

template <class T, class Policy>
T tgamma_ratio_imp(T x, T y, const Policy& pol)
{
   BOOST_MATH_STD_USING

   if((x <= 0) || (boost::math::isinf)(x))
      return policies::raise_domain_error<T>("boost::math::tgamma_ratio<%1%>(%1%, %1%)", "Gamma function ratios only implemented for positive arguments (got a=%1%).", x, pol);
   if((y <= 0) || (boost::math::isinf)(y))
      return policies::raise_domain_error<T>("boost::math::tgamma_ratio<%1%>(%1%, %1%)", "Gamma function ratios only implemented for positive arguments (got b=%1%).", y, pol);

   if(x <= tools::min_value<T>())
   {
      // Special case for denorms...Ugh.
      T shift = ldexp(T(1), tools::digits<T>());
      return shift * tgamma_ratio_imp(T(x * shift), y, pol);
   }

   if((x < max_factorial<T>::value) && (y < max_factorial<T>::value))
   {
      // Rather than subtracting values, lets just call the gamma functions directly:
      return boost::math::tgamma(x, pol) / boost::math::tgamma(y, pol);
   }
   T prefix = 1;
   if(x < 1)
   {
      if(y < 2 * max_factorial<T>::value)
      {
         // We need to sidestep on x as well, otherwise we'll underflow
         // before we get to factor in the prefix term:
         prefix /= x;
         x += 1;
         while(y >=  max_factorial<T>::value)
         {
            y -= 1;
            prefix /= y;
         }
         return prefix * boost::math::tgamma(x, pol) / boost::math::tgamma(y, pol);
      }
      //
      // result is almost certainly going to underflow to zero, try logs just in case:
      //
      return exp(boost::math::lgamma(x, pol) - boost::math::lgamma(y, pol));
   }
   if(y < 1)
   {
      if(x < 2 * max_factorial<T>::value)
      {
         // We need to sidestep on y as well, otherwise we'll overflow
         // before we get to factor in the prefix term:
         prefix *= y;
         y += 1;
         while(x >= max_factorial<T>::value)
         {
            x -= 1;
            prefix *= x;
         }
         return prefix * boost::math::tgamma(x, pol) / boost::math::tgamma(y, pol);
      }
      //
      // Result will almost certainly overflow, try logs just in case:
      //
      return exp(boost::math::lgamma(x, pol) - boost::math::lgamma(y, pol));
   }
   //
   // Regular case, x and y both large and similar in magnitude:
   //
   return boost::math::tgamma_delta_ratio(x, y - x, pol);
}

template <class T, class Policy>
T gamma_p_derivative_imp(T a, T x, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Usual error checks first:
   //
   if(a <= 0)
      return policies::raise_domain_error<T>("boost::math::gamma_p_derivative<%1%>(%1%, %1%)", "Argument a to the incomplete gamma function must be greater than zero (got a=%1%).", a, pol);
   if(x < 0)
      return policies::raise_domain_error<T>("boost::math::gamma_p_derivative<%1%>(%1%, %1%)", "Argument x to the incomplete gamma function must be >= 0 (got x=%1%).", x, pol);
   //
   // Now special cases:
   //
   if(x == 0)
   {
      return (a > 1) ? 0 :
         (a == 1) ? 1 : policies::raise_overflow_error<T>("boost::math::gamma_p_derivative<%1%>(%1%, %1%)", 0, pol);
   }
   //
   // Normal case:
   //
   typedef typename lanczos::lanczos<T, Policy>::type lanczos_type;
   T f1 = detail::regularised_gamma_prefix(a, x, pol, lanczos_type());
   if((x < 1) && (tools::max_value<T>() * x < f1))
   {
      // overflow:
      return policies::raise_overflow_error<T>("boost::math::gamma_p_derivative<%1%>(%1%, %1%)", 0, pol);
   }
   if(f1 == 0)
   {
      // Underflow in calculation, use logs instead:
      f1 = a * log(x) - x - lgamma(a, pol) - log(x);
      f1 = exp(f1);
   }
   else
      f1 /= x;

   return f1;
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type 
   tgamma(T z, const Policy& /* pol */, const std::true_type)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;
   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::gamma_imp(static_cast<value_type>(z), forwarding_policy(), evaluation_type()), "boost::math::tgamma<%1%>(%1%)");
}

template <class T, class Policy>
struct igamma_initializer
{
   struct init
   {
      init()
      {
         typedef typename policies::precision<T, Policy>::type precision_type;

         typedef std::integral_constant<int,
            precision_type::value <= 0 ? 0 :
            precision_type::value <= 53 ? 53 :
            precision_type::value <= 64 ? 64 :
            precision_type::value <= 113 ? 113 : 0
         > tag_type;

         do_init(tag_type());
      }
      template <int N>
      static void do_init(const std::integral_constant<int, N>&)
      {
         // If std::numeric_limits<T>::digits is zero, we must not call
         // our initialization code here as the precision presumably
         // varies at runtime, and will not have been set yet.  Plus the
         // code requiring initialization isn't called when digits == 0.
         if(std::numeric_limits<T>::digits)
         {
            boost::math::gamma_p(static_cast<T>(400), static_cast<T>(400), Policy());
         }
      }
      static void do_init(const std::integral_constant<int, 53>&){}
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class Policy>
const typename igamma_initializer<T, Policy>::init igamma_initializer<T, Policy>::initializer;

template <class T, class Policy>
struct lgamma_initializer
{
   struct init
   {
      init()
      {
         typedef typename policies::precision<T, Policy>::type precision_type;
         typedef std::integral_constant<int,
            precision_type::value <= 0 ? 0 :
            precision_type::value <= 64 ? 64 :
            precision_type::value <= 113 ? 113 : 0
         > tag_type;

         do_init(tag_type());
      }
      static void do_init(const std::integral_constant<int, 64>&)
      {
         boost::math::lgamma(static_cast<T>(2.5), Policy());
         boost::math::lgamma(static_cast<T>(1.25), Policy());
         boost::math::lgamma(static_cast<T>(1.75), Policy());
      }
      static void do_init(const std::integral_constant<int, 113>&)
      {
         boost::math::lgamma(static_cast<T>(2.5), Policy());
         boost::math::lgamma(static_cast<T>(1.25), Policy());
         boost::math::lgamma(static_cast<T>(1.5), Policy());
         boost::math::lgamma(static_cast<T>(1.75), Policy());
      }
      static void do_init(const std::integral_constant<int, 0>&)
      {
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T, class Policy>
const typename lgamma_initializer<T, Policy>::init lgamma_initializer<T, Policy>::initializer;

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type
   tgamma(T1 a, T2 z, const Policy&, const std::false_type)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   // typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   igamma_initializer<value_type, forwarding_policy>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::gamma_incomplete_imp(static_cast<value_type>(a),
      static_cast<value_type>(z), false, true,
      forwarding_policy(), static_cast<value_type*>(0)), "boost::math::tgamma<%1%>(%1%, %1%)");
}

template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type
   tgamma(T1 a, T2 z, const std::false_type& tag)
{
   return tgamma(a, z, policies::policy<>(), tag);
}


} // namespace detail

template <class T>
inline typename tools::promote_args<T>::type 
   tgamma(T z)
{
   return tgamma(z, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type 
   lgamma(T z, int* sign, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   detail::lgamma_initializer<value_type, forwarding_policy>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::lgamma_imp(static_cast<value_type>(z), forwarding_policy(), evaluation_type(), sign), "boost::math::lgamma<%1%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type 
   lgamma(T z, int* sign)
{
   return lgamma(z, sign, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type 
   lgamma(T x, const Policy& pol)
{
   return ::boost::math::lgamma(x, 0, pol);
}

template <class T>
inline typename tools::promote_args<T>::type 
   lgamma(T x)
{
   return ::boost::math::lgamma(x, 0, policies::policy<>());
}

template <class T, class Policy>
inline typename tools::promote_args<T>::type 
   tgamma1pm1(T z, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<typename std::remove_cv<result_type>::type, forwarding_policy>(detail::tgammap1m1_imp(static_cast<value_type>(z), forwarding_policy(), evaluation_type()), "boost::math::tgamma1pm1<%!%>(%1%)");
}

template <class T>
inline typename tools::promote_args<T>::type 
   tgamma1pm1(T z)
{
   return tgamma1pm1(z, policies::policy<>());
}

//
// Full upper incomplete gamma:
//
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type
   tgamma(T1 a, T2 z)
{
   //
   // Type T2 could be a policy object, or a value, select the 
   // right overload based on T2:
   //
   typedef typename policies::is_policy<T2>::type maybe_policy;
   return detail::tgamma(a, z, maybe_policy());
}
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type
   tgamma(T1 a, T2 z, const Policy& pol)
{
   return detail::tgamma(a, z, pol, std::false_type());
}
//
// Full lower incomplete gamma:
//
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type
   tgamma_lower(T1 a, T2 z, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   // typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   detail::igamma_initializer<value_type, forwarding_policy>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::gamma_incomplete_imp(static_cast<value_type>(a),
      static_cast<value_type>(z), false, false,
      forwarding_policy(), static_cast<value_type*>(0)), "tgamma_lower<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type
   tgamma_lower(T1 a, T2 z)
{
   return tgamma_lower(a, z, policies::policy<>());
}
//
// Regularised upper incomplete gamma:
//
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type
   gamma_q(T1 a, T2 z, const Policy& /* pol */)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   // typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   detail::igamma_initializer<value_type, forwarding_policy>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::gamma_incomplete_imp(static_cast<value_type>(a),
      static_cast<value_type>(z), true, true,
      forwarding_policy(), static_cast<value_type*>(0)), "gamma_q<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type
   gamma_q(T1 a, T2 z)
{
   return gamma_q(a, z, policies::policy<>());
}
//
// Regularised lower incomplete gamma:
//
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type
   gamma_p(T1 a, T2 z, const Policy&)
{
   BOOST_FPU_EXCEPTION_GUARD
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   // typedef typename lanczos::lanczos<value_type, Policy>::type evaluation_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   detail::igamma_initializer<value_type, forwarding_policy>::force_instantiate();

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(
      detail::gamma_incomplete_imp(static_cast<value_type>(a),
      static_cast<value_type>(z), true, false,
      forwarding_policy(), static_cast<value_type*>(0)), "gamma_p<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type
   gamma_p(T1 a, T2 z)
{
   return gamma_p(a, z, policies::policy<>());
}

// ratios of gamma functions:
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   tgamma_delta_ratio(T1 z, T2 delta, const Policy& /* pol */)
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

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::tgamma_delta_ratio_imp(static_cast<value_type>(z), static_cast<value_type>(delta), forwarding_policy()), "boost::math::tgamma_delta_ratio<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   tgamma_delta_ratio(T1 z, T2 delta)
{
   return tgamma_delta_ratio(z, delta, policies::policy<>());
}
template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   tgamma_ratio(T1 a, T2 b, const Policy&)
{
   typedef typename tools::promote_args<T1, T2>::type result_type;
   typedef typename policies::evaluation<result_type, Policy>::type value_type;
   typedef typename policies::normalise<
      Policy, 
      policies::promote_float<false>, 
      policies::promote_double<false>, 
      policies::discrete_quantile<>,
      policies::assert_undefined<> >::type forwarding_policy;

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::tgamma_ratio_imp(static_cast<value_type>(a), static_cast<value_type>(b), forwarding_policy()), "boost::math::tgamma_delta_ratio<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   tgamma_ratio(T1 a, T2 b)
{
   return tgamma_ratio(a, b, policies::policy<>());
}

template <class T1, class T2, class Policy>
inline typename tools::promote_args<T1, T2>::type 
   gamma_p_derivative(T1 a, T2 x, const Policy&)
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

   return policies::checked_narrowing_cast<result_type, forwarding_policy>(detail::gamma_p_derivative_imp(static_cast<value_type>(a), static_cast<value_type>(x), forwarding_policy()), "boost::math::gamma_p_derivative<%1%>(%1%, %1%)");
}
template <class T1, class T2>
inline typename tools::promote_args<T1, T2>::type 
   gamma_p_derivative(T1 a, T2 x)
{
   return gamma_p_derivative(a, x, policies::policy<>());
}

} // namespace math
} // namespace boost

#ifdef _MSC_VER
# pragma warning(pop)
#endif

#include <boost/math/special_functions/detail/igamma_inverse.hpp>
#include <boost/math/special_functions/detail/gamma_inva.hpp>
#include <boost/math/special_functions/erf.hpp>

#endif // BOOST_MATH_SF_GAMMA_HPP
