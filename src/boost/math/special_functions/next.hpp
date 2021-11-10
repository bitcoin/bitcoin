//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_SPECIAL_NEXT_HPP
#define BOOST_MATH_SPECIAL_NEXT_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/policies/error_handling.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <boost/math/special_functions/trunc.hpp>
#include <boost/math/tools/traits.hpp>
#include <type_traits>
#include <cfloat>


#if !defined(_CRAYC) && !defined(__CUDACC__) && (!defined(__GNUC__) || (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ > 3)))
#if (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || defined(__SSE2__)
#include "xmmintrin.h"
#define BOOST_MATH_CHECK_SSE2
#endif
#endif

namespace boost{ namespace math{

   namespace concepts {

      class real_concept;
      class std_real_concept;

   }

namespace detail{

template <class T>
struct has_hidden_guard_digits;
template <>
struct has_hidden_guard_digits<float> : public std::false_type {};
template <>
struct has_hidden_guard_digits<double> : public std::false_type {};
template <>
struct has_hidden_guard_digits<long double> : public std::false_type {};
#ifdef BOOST_HAS_FLOAT128
template <>
struct has_hidden_guard_digits<__float128> : public std::false_type {};
#endif
template <>
struct has_hidden_guard_digits<boost::math::concepts::real_concept> : public std::false_type {};
template <>
struct has_hidden_guard_digits<boost::math::concepts::std_real_concept> : public std::false_type {};

template <class T, bool b>
struct has_hidden_guard_digits_10 : public std::false_type {};
template <class T>
struct has_hidden_guard_digits_10<T, true> : public std::integral_constant<bool, (std::numeric_limits<T>::digits10 != std::numeric_limits<T>::max_digits10)> {};

template <class T>
struct has_hidden_guard_digits 
   : public has_hidden_guard_digits_10<T, 
   std::numeric_limits<T>::is_specialized
   && (std::numeric_limits<T>::radix == 10) >
{};

template <class T>
inline const T& normalize_value(const T& val, const std::false_type&) { return val; }
template <class T>
inline T normalize_value(const T& val, const std::true_type&) 
{
   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   std::intmax_t shift = (std::intmax_t)std::numeric_limits<T>::digits - (std::intmax_t)ilogb(val) - 1;
   T result = scalbn(val, shift);
   result = round(result);
   return scalbn(result, -shift); 
}

template <class T>
inline T get_smallest_value(std::true_type const&)
{
   //
   // numeric_limits lies about denorms being present - particularly
   // when this can be turned on or off at runtime, as is the case
   // when using the SSE2 registers in DAZ or FTZ mode.
   //
   static const T m = std::numeric_limits<T>::denorm_min();
#ifdef BOOST_MATH_CHECK_SSE2
   return (_mm_getcsr() & (_MM_FLUSH_ZERO_ON | 0x40)) ? tools::min_value<T>() : m;
#else
   return ((tools::min_value<T>() / 2) == 0) ? tools::min_value<T>() : m;
#endif
}

template <class T>
inline T get_smallest_value(std::false_type const&)
{
   return tools::min_value<T>();
}

template <class T>
inline T get_smallest_value()
{
#if defined(BOOST_MSVC) && (BOOST_MSVC <= 1310)
   return get_smallest_value<T>(std::integral_constant<bool, std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_denorm == 1)>());
#else
   return get_smallest_value<T>(std::integral_constant<bool, std::numeric_limits<T>::is_specialized && (std::numeric_limits<T>::has_denorm == std::denorm_present)>());
#endif
}

//
// Returns the smallest value that won't generate denorms when
// we calculate the value of the least-significant-bit:
//
template <class T>
T get_min_shift_value();

template <class T>
struct min_shift_initializer
{
   struct init
   {
      init()
      {
         do_init();
      }
      static void do_init()
      {
         get_min_shift_value<T>();
      }
      void force_instantiate()const{}
   };
   static const init initializer;
   static void force_instantiate()
   {
      initializer.force_instantiate();
   }
};

template <class T>
const typename min_shift_initializer<T>::init min_shift_initializer<T>::initializer;

template <class T>
inline T calc_min_shifted(const std::true_type&)
{
   BOOST_MATH_STD_USING
   return ldexp(tools::min_value<T>(), tools::digits<T>() + 1);
}
template <class T>
inline T calc_min_shifted(const std::false_type&)
{
   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   return scalbn(tools::min_value<T>(), std::numeric_limits<T>::digits + 1);
}


template <class T>
inline T get_min_shift_value()
{
   static const T val = calc_min_shifted<T>(std::integral_constant<bool, !std::numeric_limits<T>::is_specialized || std::numeric_limits<T>::radix == 2>());
   min_shift_initializer<T>::force_instantiate();

   return val;
}

template <class T, bool b = boost::math::tools::detail::has_backend_type<T>::value>
struct exponent_type
{
   typedef int type;
};

template <class T>
struct exponent_type<T, true>
{
   typedef typename T::backend_type::exponent_type type;
};

template <class T, class Policy>
T float_next_imp(const T& val, const std::true_type&, const Policy& pol)
{
   typedef typename exponent_type<T>::type exponent_type;
   
   BOOST_MATH_STD_USING
   exponent_type expon;
   static const char* function = "float_next<%1%>(%1%)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
   {
      if(val < 0)
         return -tools::max_value<T>();
      return policies::raise_domain_error<T>(
         function,
         "Argument must be finite, but got %1%", val, pol);
   }

   if(val >= tools::max_value<T>())
      return policies::raise_overflow_error<T>(function, 0, pol);

   if(val == 0)
      return detail::get_smallest_value<T>();

   if((fpclass != (int)FP_SUBNORMAL) && (fpclass != (int)FP_ZERO) && (fabs(val) < detail::get_min_shift_value<T>()) && (val != -tools::min_value<T>()))
   {
      //
      // Special case: if the value of the least significant bit is a denorm, and the result
      // would not be a denorm, then shift the input, increment, and shift back.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      return ldexp(float_next(T(ldexp(val, 2 * tools::digits<T>())), pol), -2 * tools::digits<T>());
   }

   if(-0.5f == frexp(val, &expon))
      --expon; // reduce exponent when val is a power of two, and negative.
   T diff = ldexp(T(1), expon - tools::digits<T>());
   if(diff == 0)
      diff = detail::get_smallest_value<T>();
   return val + diff;
} // float_next_imp
//
// Special version for some base other than 2:
//
template <class T, class Policy>
T float_next_imp(const T& val, const std::false_type&, const Policy& pol)
{
   typedef typename exponent_type<T>::type exponent_type;

   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   BOOST_MATH_STD_USING
   exponent_type expon;
   static const char* function = "float_next<%1%>(%1%)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
   {
      if(val < 0)
         return -tools::max_value<T>();
      return policies::raise_domain_error<T>(
         function,
         "Argument must be finite, but got %1%", val, pol);
   }

   if(val >= tools::max_value<T>())
      return policies::raise_overflow_error<T>(function, 0, pol);

   if(val == 0)
      return detail::get_smallest_value<T>();

   if((fpclass != (int)FP_SUBNORMAL) && (fpclass != (int)FP_ZERO) && (fabs(val) < detail::get_min_shift_value<T>()) && (val != -tools::min_value<T>()))
   {
      //
      // Special case: if the value of the least significant bit is a denorm, and the result
      // would not be a denorm, then shift the input, increment, and shift back.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      return scalbn(float_next(T(scalbn(val, 2 * std::numeric_limits<T>::digits)), pol), -2 * std::numeric_limits<T>::digits);
   }

   expon = 1 + ilogb(val);
   if(-1 == scalbn(val, -expon) * std::numeric_limits<T>::radix)
      --expon; // reduce exponent when val is a power of base, and negative.
   T diff = scalbn(T(1), expon - std::numeric_limits<T>::digits);
   if(diff == 0)
      diff = detail::get_smallest_value<T>();
   return val + diff;
} // float_next_imp

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type float_next(const T& val, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   return detail::float_next_imp(detail::normalize_value(static_cast<result_type>(val), typename detail::has_hidden_guard_digits<result_type>::type()), std::integral_constant<bool, !std::numeric_limits<result_type>::is_specialized || (std::numeric_limits<result_type>::radix == 2)>(), pol);
}

#if 0 //def BOOST_MSVC
//
// We used to use ::_nextafter here, but doing so fails when using
// the SSE2 registers if the FTZ or DAZ flags are set, so use our own
// - albeit slower - code instead as at least that gives the correct answer.
//
template <class Policy>
inline double float_next(const double& val, const Policy& pol)
{
   static const char* function = "float_next<%1%>(%1%)";

   if(!(boost::math::isfinite)(val) && (val > 0))
      return policies::raise_domain_error<double>(
         function,
         "Argument must be finite, but got %1%", val, pol);

   if(val >= tools::max_value<double>())
      return policies::raise_overflow_error<double>(function, 0, pol);

   return ::_nextafter(val, tools::max_value<double>());
}
#endif

template <class T>
inline typename tools::promote_args<T>::type float_next(const T& val)
{
   return float_next(val, policies::policy<>());
}

namespace detail{

template <class T, class Policy>
T float_prior_imp(const T& val, const std::true_type&, const Policy& pol)
{
   typedef typename exponent_type<T>::type exponent_type;

   BOOST_MATH_STD_USING
   exponent_type expon;
   static const char* function = "float_prior<%1%>(%1%)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
   {
      if(val > 0)
         return tools::max_value<T>();
      return policies::raise_domain_error<T>(
         function,
         "Argument must be finite, but got %1%", val, pol);
   }

   if(val <= -tools::max_value<T>())
      return -policies::raise_overflow_error<T>(function, 0, pol);

   if(val == 0)
      return -detail::get_smallest_value<T>();

   if((fpclass != (int)FP_SUBNORMAL) && (fpclass != (int)FP_ZERO) && (fabs(val) < detail::get_min_shift_value<T>()) && (val != tools::min_value<T>()))
   {
      //
      // Special case: if the value of the least significant bit is a denorm, and the result
      // would not be a denorm, then shift the input, increment, and shift back.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      return ldexp(float_prior(T(ldexp(val, 2 * tools::digits<T>())), pol), -2 * tools::digits<T>());
   }

   T remain = frexp(val, &expon);
   if(remain == 0.5f)
      --expon; // when val is a power of two we must reduce the exponent
   T diff = ldexp(T(1), expon - tools::digits<T>());
   if(diff == 0)
      diff = detail::get_smallest_value<T>();
   return val - diff;
} // float_prior_imp
//
// Special version for bases other than 2:
//
template <class T, class Policy>
T float_prior_imp(const T& val, const std::false_type&, const Policy& pol)
{
   typedef typename exponent_type<T>::type exponent_type;

   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   BOOST_MATH_STD_USING
   exponent_type expon;
   static const char* function = "float_prior<%1%>(%1%)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
   {
      if(val > 0)
         return tools::max_value<T>();
      return policies::raise_domain_error<T>(
         function,
         "Argument must be finite, but got %1%", val, pol);
   }

   if(val <= -tools::max_value<T>())
      return -policies::raise_overflow_error<T>(function, 0, pol);

   if(val == 0)
      return -detail::get_smallest_value<T>();

   if((fpclass != (int)FP_SUBNORMAL) && (fpclass != (int)FP_ZERO) && (fabs(val) < detail::get_min_shift_value<T>()) && (val != tools::min_value<T>()))
   {
      //
      // Special case: if the value of the least significant bit is a denorm, and the result
      // would not be a denorm, then shift the input, increment, and shift back.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      return scalbn(float_prior(T(scalbn(val, 2 * std::numeric_limits<T>::digits)), pol), -2 * std::numeric_limits<T>::digits);
   }

   expon = 1 + ilogb(val);
   T remain = scalbn(val, -expon);
   if(remain * std::numeric_limits<T>::radix == 1)
      --expon; // when val is a power of two we must reduce the exponent
   T diff = scalbn(T(1), expon - std::numeric_limits<T>::digits);
   if(diff == 0)
      diff = detail::get_smallest_value<T>();
   return val - diff;
} // float_prior_imp

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type float_prior(const T& val, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   return detail::float_prior_imp(detail::normalize_value(static_cast<result_type>(val), typename detail::has_hidden_guard_digits<result_type>::type()), std::integral_constant<bool, !std::numeric_limits<result_type>::is_specialized || (std::numeric_limits<result_type>::radix == 2)>(), pol);
}

#if 0 //def BOOST_MSVC
//
// We used to use ::_nextafter here, but doing so fails when using
// the SSE2 registers if the FTZ or DAZ flags are set, so use our own
// - albeit slower - code instead as at least that gives the correct answer.
//
template <class Policy>
inline double float_prior(const double& val, const Policy& pol)
{
   static const char* function = "float_prior<%1%>(%1%)";

   if(!(boost::math::isfinite)(val) && (val < 0))
      return policies::raise_domain_error<double>(
         function,
         "Argument must be finite, but got %1%", val, pol);

   if(val <= -tools::max_value<double>())
      return -policies::raise_overflow_error<double>(function, 0, pol);

   return ::_nextafter(val, -tools::max_value<double>());
}
#endif

template <class T>
inline typename tools::promote_args<T>::type float_prior(const T& val)
{
   return float_prior(val, policies::policy<>());
}

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type nextafter(const T& val, const U& direction, const Policy& pol)
{
   typedef typename tools::promote_args<T, U>::type result_type;
   return val < direction ? boost::math::float_next<result_type>(val, pol) : val == direction ? val : boost::math::float_prior<result_type>(val, pol);
}

template <class T, class U>
inline typename tools::promote_args<T, U>::type nextafter(const T& val, const U& direction)
{
   return nextafter(val, direction, policies::policy<>());
}

namespace detail{

template <class T, class Policy>
T float_distance_imp(const T& a, const T& b, const std::true_type&, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Error handling:
   //
   static const char* function = "float_distance<%1%>(%1%, %1%)";
   if(!(boost::math::isfinite)(a))
      return policies::raise_domain_error<T>(
         function,
         "Argument a must be finite, but got %1%", a, pol);
   if(!(boost::math::isfinite)(b))
      return policies::raise_domain_error<T>(
         function,
         "Argument b must be finite, but got %1%", b, pol);
   //
   // Special cases:
   //
   if(a > b)
      return -float_distance(b, a, pol);
   if(a == b)
      return T(0);
   if(a == 0)
      return 1 + fabs(float_distance(static_cast<T>((b < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), b, pol));
   if(b == 0)
      return 1 + fabs(float_distance(static_cast<T>((a < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), a, pol));
   if(boost::math::sign(a) != boost::math::sign(b))
      return 2 + fabs(float_distance(static_cast<T>((b < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), b, pol))
         + fabs(float_distance(static_cast<T>((a < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), a, pol));
   //
   // By the time we get here, both a and b must have the same sign, we want
   // b > a and both positive for the following logic:
   //
   if(a < 0)
      return float_distance(static_cast<T>(-b), static_cast<T>(-a), pol);

   BOOST_MATH_ASSERT(a >= 0);
   BOOST_MATH_ASSERT(b >= a);

   int expon;
   //
   // Note that if a is a denorm then the usual formula fails
   // because we actually have fewer than tools::digits<T>()
   // significant bits in the representation:
   //
   (void)frexp(((boost::math::fpclassify)(a) == (int)FP_SUBNORMAL) ? tools::min_value<T>() : a, &expon);
   T upper = ldexp(T(1), expon);
   T result = T(0);
   //
   // If b is greater than upper, then we *must* split the calculation
   // as the size of the ULP changes with each order of magnitude change:
   //
   if(b > upper)
   {
      int expon2;
      (void)frexp(b, &expon2);
      T upper2 = ldexp(T(0.5), expon2);
      result = float_distance(upper2, b);
      result += (expon2 - expon - 1) * ldexp(T(1), tools::digits<T>() - 1);
   }
   //
   // Use compensated double-double addition to avoid rounding
   // errors in the subtraction:
   //
   expon = tools::digits<T>() - expon;
   T mb, x, y, z;
   if(((boost::math::fpclassify)(a) == (int)FP_SUBNORMAL) || (b - a < tools::min_value<T>()))
   {
      //
      // Special case - either one end of the range is a denormal, or else the difference is.
      // The regular code will fail if we're using the SSE2 registers on Intel and either
      // the FTZ or DAZ flags are set.
      //
      T a2 = ldexp(a, tools::digits<T>());
      T b2 = ldexp(b, tools::digits<T>());
      mb = -(std::min)(T(ldexp(upper, tools::digits<T>())), b2);
      x = a2 + mb;
      z = x - a2;
      y = (a2 - (x - z)) + (mb - z);

      expon -= tools::digits<T>();
   }
   else
   {
      mb = -(std::min)(upper, b);
      x = a + mb;
      z = x - a;
      y = (a - (x - z)) + (mb - z);
   }
   if(x < 0)
   {
      x = -x;
      y = -y;
   }
   result += ldexp(x, expon) + ldexp(y, expon);
   //
   // Result must be an integer:
   //
   BOOST_MATH_ASSERT(result == floor(result));
   return result;
} // float_distance_imp
//
// Special versions for bases other than 2:
//
template <class T, class Policy>
T float_distance_imp(const T& a, const T& b, const std::false_type&, const Policy& pol)
{
   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   BOOST_MATH_STD_USING
   //
   // Error handling:
   //
   static const char* function = "float_distance<%1%>(%1%, %1%)";
   if(!(boost::math::isfinite)(a))
      return policies::raise_domain_error<T>(
         function,
         "Argument a must be finite, but got %1%", a, pol);
   if(!(boost::math::isfinite)(b))
      return policies::raise_domain_error<T>(
         function,
         "Argument b must be finite, but got %1%", b, pol);
   //
   // Special cases:
   //
   if(a > b)
      return -float_distance(b, a, pol);
   if(a == b)
      return T(0);
   if(a == 0)
      return 1 + fabs(float_distance(static_cast<T>((b < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), b, pol));
   if(b == 0)
      return 1 + fabs(float_distance(static_cast<T>((a < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), a, pol));
   if(boost::math::sign(a) != boost::math::sign(b))
      return 2 + fabs(float_distance(static_cast<T>((b < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), b, pol))
         + fabs(float_distance(static_cast<T>((a < 0) ? T(-detail::get_smallest_value<T>()) : detail::get_smallest_value<T>()), a, pol));
   //
   // By the time we get here, both a and b must have the same sign, we want
   // b > a and both positive for the following logic:
   //
   if(a < 0)
      return float_distance(static_cast<T>(-b), static_cast<T>(-a), pol);

   BOOST_MATH_ASSERT(a >= 0);
   BOOST_MATH_ASSERT(b >= a);

   std::intmax_t expon;
   //
   // Note that if a is a denorm then the usual formula fails
   // because we actually have fewer than tools::digits<T>()
   // significant bits in the representation:
   //
   expon = 1 + ilogb(((boost::math::fpclassify)(a) == (int)FP_SUBNORMAL) ? tools::min_value<T>() : a);
   T upper = scalbn(T(1), expon);
   T result = T(0);
   //
   // If b is greater than upper, then we *must* split the calculation
   // as the size of the ULP changes with each order of magnitude change:
   //
   if(b > upper)
   {
      std::intmax_t expon2 = 1 + ilogb(b);
      T upper2 = scalbn(T(1), expon2 - 1);
      result = float_distance(upper2, b);
      result += (expon2 - expon - 1) * scalbn(T(1), std::numeric_limits<T>::digits - 1);
   }
   //
   // Use compensated double-double addition to avoid rounding
   // errors in the subtraction:
   //
   expon = std::numeric_limits<T>::digits - expon;
   T mb, x, y, z;
   if(((boost::math::fpclassify)(a) == (int)FP_SUBNORMAL) || (b - a < tools::min_value<T>()))
   {
      //
      // Special case - either one end of the range is a denormal, or else the difference is.
      // The regular code will fail if we're using the SSE2 registers on Intel and either
      // the FTZ or DAZ flags are set.
      //
      T a2 = scalbn(a, std::numeric_limits<T>::digits);
      T b2 = scalbn(b, std::numeric_limits<T>::digits);
      mb = -(std::min)(T(scalbn(upper, std::numeric_limits<T>::digits)), b2);
      x = a2 + mb;
      z = x - a2;
      y = (a2 - (x - z)) + (mb - z);

      expon -= std::numeric_limits<T>::digits;
   }
   else
   {
      mb = -(std::min)(upper, b);
      x = a + mb;
      z = x - a;
      y = (a - (x - z)) + (mb - z);
   }
   if(x < 0)
   {
      x = -x;
      y = -y;
   }
   result += scalbn(x, expon) + scalbn(y, expon);
   //
   // Result must be an integer:
   //
   BOOST_MATH_ASSERT(result == floor(result));
   return result;
} // float_distance_imp

} // namespace detail

template <class T, class U, class Policy>
inline typename tools::promote_args<T, U>::type float_distance(const T& a, const U& b, const Policy& pol)
{
   //
   // We allow ONE of a and b to be an integer type, otherwise both must be the SAME type.
   //
   static_assert(
      (std::is_same<T, U>::value 
      || (std::is_integral<T>::value && !std::is_integral<U>::value) 
      || (!std::is_integral<T>::value && std::is_integral<U>::value)
      || (std::numeric_limits<T>::is_specialized && std::numeric_limits<U>::is_specialized
         && (std::numeric_limits<T>::digits == std::numeric_limits<U>::digits)
         && (std::numeric_limits<T>::radix == std::numeric_limits<U>::radix)
         && !std::numeric_limits<T>::is_integer && !std::numeric_limits<U>::is_integer)),
      "Float distance between two different floating point types is undefined.");

   BOOST_IF_CONSTEXPR (!std::is_same<T, U>::value)
   {
      BOOST_IF_CONSTEXPR(std::is_integral<T>::value)
      {
         return float_distance(static_cast<U>(a), b, pol);
      }
      else
      {
         return float_distance(a, static_cast<T>(b), pol);
      }
   }
   else
   {
      typedef typename tools::promote_args<T, U>::type result_type;
      return detail::float_distance_imp(detail::normalize_value(static_cast<result_type>(a), typename detail::has_hidden_guard_digits<result_type>::type()), detail::normalize_value(static_cast<result_type>(b), typename detail::has_hidden_guard_digits<result_type>::type()), std::integral_constant<bool, !std::numeric_limits<result_type>::is_specialized || (std::numeric_limits<result_type>::radix == 2)>(), pol);
   }
}

template <class T, class U>
typename tools::promote_args<T, U>::type float_distance(const T& a, const U& b)
{
   return boost::math::float_distance(a, b, policies::policy<>());
}

namespace detail{

template <class T, class Policy>
T float_advance_imp(T val, int distance, const std::true_type&, const Policy& pol)
{
   BOOST_MATH_STD_USING
   //
   // Error handling:
   //
   static const char* function = "float_advance<%1%>(%1%, int)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
      return policies::raise_domain_error<T>(
         function,
         "Argument val must be finite, but got %1%", val, pol);

   if(val < 0)
      return -float_advance(-val, -distance, pol);
   if(distance == 0)
      return val;
   if(distance == 1)
      return float_next(val, pol);
   if(distance == -1)
      return float_prior(val, pol);

   if(fabs(val) < detail::get_min_shift_value<T>())
   {
      //
      // Special case: if the value of the least significant bit is a denorm,
      // implement in terms of float_next/float_prior.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      if(distance > 0)
      {
         do{ val = float_next(val, pol); } while(--distance);
      }
      else
      {
         do{ val = float_prior(val, pol); } while(++distance);
      }
      return val;
   }

   int expon;
   (void)frexp(val, &expon);
   T limit = ldexp((distance < 0 ? T(0.5f) : T(1)), expon);
   if(val <= tools::min_value<T>())
   {
      limit = sign(T(distance)) * tools::min_value<T>();
   }
   T limit_distance = float_distance(val, limit);
   while(fabs(limit_distance) < abs(distance))
   {
      distance -= itrunc(limit_distance);
      val = limit;
      if(distance < 0)
      {
         limit /= 2;
         expon--;
      }
      else
      {
         limit *= 2;
         expon++;
      }
      limit_distance = float_distance(val, limit);
      if(distance && (limit_distance == 0))
      {
         return policies::raise_evaluation_error<T>(function, "Internal logic failed while trying to increment floating point value %1%: most likely your FPU is in non-IEEE conforming mode.", val, pol);
      }
   }
   if((0.5f == frexp(val, &expon)) && (distance < 0))
      --expon;
   T diff = 0;
   if(val != 0)
      diff = distance * ldexp(T(1), expon - tools::digits<T>());
   if(diff == 0)
      diff = distance * detail::get_smallest_value<T>();
   return val += diff;
} // float_advance_imp
//
// Special version for bases other than 2:
//
template <class T, class Policy>
T float_advance_imp(T val, int distance, const std::false_type&, const Policy& pol)
{
   static_assert(std::numeric_limits<T>::is_specialized, "Type T must be specialized.");
   static_assert(std::numeric_limits<T>::radix != 2, "Type T must be specialized.");

   BOOST_MATH_STD_USING
   //
   // Error handling:
   //
   static const char* function = "float_advance<%1%>(%1%, int)";

   int fpclass = (boost::math::fpclassify)(val);

   if((fpclass == (int)FP_NAN) || (fpclass == (int)FP_INFINITE))
      return policies::raise_domain_error<T>(
         function,
         "Argument val must be finite, but got %1%", val, pol);

   if(val < 0)
      return -float_advance(-val, -distance, pol);
   if(distance == 0)
      return val;
   if(distance == 1)
      return float_next(val, pol);
   if(distance == -1)
      return float_prior(val, pol);

   if(fabs(val) < detail::get_min_shift_value<T>())
   {
      //
      // Special case: if the value of the least significant bit is a denorm,
      // implement in terms of float_next/float_prior.
      // This avoids issues with the Intel SSE2 registers when the FTZ or DAZ flags are set.
      //
      if(distance > 0)
      {
         do{ val = float_next(val, pol); } while(--distance);
      }
      else
      {
         do{ val = float_prior(val, pol); } while(++distance);
      }
      return val;
   }

   std::intmax_t expon = 1 + ilogb(val);
   T limit = scalbn(T(1), distance < 0 ? expon - 1 : expon);
   if(val <= tools::min_value<T>())
   {
      limit = sign(T(distance)) * tools::min_value<T>();
   }
   T limit_distance = float_distance(val, limit);
   while(fabs(limit_distance) < abs(distance))
   {
      distance -= itrunc(limit_distance);
      val = limit;
      if(distance < 0)
      {
         limit /= std::numeric_limits<T>::radix;
         expon--;
      }
      else
      {
         limit *= std::numeric_limits<T>::radix;
         expon++;
      }
      limit_distance = float_distance(val, limit);
      if(distance && (limit_distance == 0))
      {
         return policies::raise_evaluation_error<T>(function, "Internal logic failed while trying to increment floating point value %1%: most likely your FPU is in non-IEEE conforming mode.", val, pol);
      }
   }
   /*expon = 1 + ilogb(val);
   if((1 == scalbn(val, 1 + expon)) && (distance < 0))
      --expon;*/
   T diff = 0;
   if(val != 0)
      diff = distance * scalbn(T(1), expon - std::numeric_limits<T>::digits);
   if(diff == 0)
      diff = distance * detail::get_smallest_value<T>();
   return val += diff;
} // float_advance_imp

} // namespace detail

template <class T, class Policy>
inline typename tools::promote_args<T>::type float_advance(T val, int distance, const Policy& pol)
{
   typedef typename tools::promote_args<T>::type result_type;
   return detail::float_advance_imp(detail::normalize_value(static_cast<result_type>(val), typename detail::has_hidden_guard_digits<result_type>::type()), distance, std::integral_constant<bool, !std::numeric_limits<result_type>::is_specialized || (std::numeric_limits<result_type>::radix == 2)>(), pol);
}

template <class T>
inline typename tools::promote_args<T>::type float_advance(const T& val, int distance)
{
   return boost::math::float_advance(val, distance, policies::policy<>());
}

}} // boost math namespaces

#endif // BOOST_MATH_SPECIAL_NEXT_HPP
