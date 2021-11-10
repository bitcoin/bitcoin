///////////////////////////////////////////////////////////////////////////////
// Copyright Christopher Kormanyos 2014.
// Copyright John Maddock 2014.
// Copyright Paul Bristow 2014.
// Distributed under the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Implement quadruple-precision <cmath> support.

#ifndef BOOST_MATH_CSTDFLOAT_CMATH_2014_02_15_HPP_
#define BOOST_MATH_CSTDFLOAT_CMATH_2014_02_15_HPP_

#include <boost/math/cstdfloat/cstdfloat_types.hpp>
#include <boost/math/cstdfloat/cstdfloat_limits.hpp>

#if defined(BOOST_CSTDFLOAT_HAS_INTERNAL_FLOAT128_T) && defined(BOOST_MATH_USE_FLOAT128) && !defined(BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT)

#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <type_traits>
#include <memory>
#include <boost/math/tools/assert.hpp>
#include <boost/math/tools/throw_exception.hpp>

#if defined(_WIN32) && defined(__GNUC__)
  // Several versions of Mingw and probably cygwin too have broken
  // libquadmath implementations that segfault as soon as you call
  // expq or any function that depends on it.
#define BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS
#endif

// Here is a helper function used for raising the value of a given
// floating-point type to the power of n, where n has integral type.
namespace boost {
   namespace math {
      namespace cstdfloat {
         namespace detail {

            template<class float_type, class integer_type>
            inline float_type pown(const float_type& x, const integer_type p)
            {
               const bool isneg = (x < 0);
               const bool isnan = (x != x);
               const bool isinf = ((!isneg) ? bool(+x > (std::numeric_limits<float_type>::max)())
                  : bool(-x > (std::numeric_limits<float_type>::max)()));

               if (isnan) { return x; }

               if (isinf) { return std::numeric_limits<float_type>::quiet_NaN(); }

               const bool       x_is_neg = (x < 0);
               const float_type abs_x = (x_is_neg ? -x : x);

               if (p < static_cast<integer_type>(0))
               {
                  if (abs_x < (std::numeric_limits<float_type>::min)())
                  {
                     return (x_is_neg ? -std::numeric_limits<float_type>::infinity()
                        : +std::numeric_limits<float_type>::infinity());
                  }
                  else
                  {
                     return float_type(1) / pown(x, static_cast<integer_type>(-p));
                  }
               }

               if (p == static_cast<integer_type>(0))
               {
                  return float_type(1);
               }
               else
               {
                  if (p == static_cast<integer_type>(1)) { return x; }

                  if (abs_x > (std::numeric_limits<float_type>::max)())
                  {
                     return (x_is_neg ? -std::numeric_limits<float_type>::infinity()
                        : +std::numeric_limits<float_type>::infinity());
                  }

                  if (p == static_cast<integer_type>(2)) { return  (x * x); }
                  else if (p == static_cast<integer_type>(3)) { return ((x * x) * x); }
                  else if (p == static_cast<integer_type>(4)) { const float_type x2 = (x * x); return (x2 * x2); }
                  else
                  {
                     // The variable xn stores the binary powers of x.
                     float_type result(((p % integer_type(2)) != integer_type(0)) ? x : float_type(1));
                     float_type xn(x);

                     integer_type p2 = p;

                     while (integer_type(p2 /= 2) != integer_type(0))
                     {
                        // Square xn for each binary power.
                        xn *= xn;

                        const bool has_binary_power = (integer_type(p2 % integer_type(2)) != integer_type(0));

                        if (has_binary_power)
                        {
                           // Multiply the result with each binary power contained in the exponent.
                           result *= xn;
                        }
                     }

                     return result;
                  }
               }
            }

         }
      }
   }
} // boost::math::cstdfloat::detail

// We will now define preprocessor symbols representing quadruple-precision <cmath> functions.
#if defined(__INTEL_COMPILER)
#define BOOST_CSTDFLOAT_FLOAT128_LDEXP  __ldexpq
#define BOOST_CSTDFLOAT_FLOAT128_FREXP  __frexpq
#define BOOST_CSTDFLOAT_FLOAT128_FABS   __fabsq
#define BOOST_CSTDFLOAT_FLOAT128_FLOOR  __floorq
#define BOOST_CSTDFLOAT_FLOAT128_CEIL   __ceilq
#if !defined(BOOST_CSTDFLOAT_FLOAT128_SQRT)
#define BOOST_CSTDFLOAT_FLOAT128_SQRT   __sqrtq
#endif
#define BOOST_CSTDFLOAT_FLOAT128_TRUNC  __truncq
#define BOOST_CSTDFLOAT_FLOAT128_EXP    __expq
#define BOOST_CSTDFLOAT_FLOAT128_EXPM1  __expm1q
#define BOOST_CSTDFLOAT_FLOAT128_POW    __powq
#define BOOST_CSTDFLOAT_FLOAT128_LOG    __logq
#define BOOST_CSTDFLOAT_FLOAT128_LOG10  __log10q
#define BOOST_CSTDFLOAT_FLOAT128_SIN    __sinq
#define BOOST_CSTDFLOAT_FLOAT128_COS    __cosq
#define BOOST_CSTDFLOAT_FLOAT128_TAN    __tanq
#define BOOST_CSTDFLOAT_FLOAT128_ASIN   __asinq
#define BOOST_CSTDFLOAT_FLOAT128_ACOS   __acosq
#define BOOST_CSTDFLOAT_FLOAT128_ATAN   __atanq
#define BOOST_CSTDFLOAT_FLOAT128_SINH   __sinhq
#define BOOST_CSTDFLOAT_FLOAT128_COSH   __coshq
#define BOOST_CSTDFLOAT_FLOAT128_TANH   __tanhq
#define BOOST_CSTDFLOAT_FLOAT128_ASINH  __asinhq
#define BOOST_CSTDFLOAT_FLOAT128_ACOSH  __acoshq
#define BOOST_CSTDFLOAT_FLOAT128_ATANH  __atanhq
#define BOOST_CSTDFLOAT_FLOAT128_FMOD   __fmodq
#define BOOST_CSTDFLOAT_FLOAT128_ATAN2  __atan2q
#define BOOST_CSTDFLOAT_FLOAT128_LGAMMA __lgammaq
#define BOOST_CSTDFLOAT_FLOAT128_TGAMMA __tgammaq
//   begin more functions
#define BOOST_CSTDFLOAT_FLOAT128_REMAINDER   __remainderq
#define BOOST_CSTDFLOAT_FLOAT128_REMQUO      __remquoq
#define BOOST_CSTDFLOAT_FLOAT128_FMA         __fmaq
#define BOOST_CSTDFLOAT_FLOAT128_FMAX        __fmaxq
#define BOOST_CSTDFLOAT_FLOAT128_FMIN        __fminq
#define BOOST_CSTDFLOAT_FLOAT128_FDIM        __fdimq
#define BOOST_CSTDFLOAT_FLOAT128_NAN         __nanq
//#define BOOST_CSTDFLOAT_FLOAT128_EXP2      __exp2q
#define BOOST_CSTDFLOAT_FLOAT128_LOG2        __log2q
#define BOOST_CSTDFLOAT_FLOAT128_LOG1P       __log1pq
#define BOOST_CSTDFLOAT_FLOAT128_CBRT        __cbrtq
#define BOOST_CSTDFLOAT_FLOAT128_HYPOT       __hypotq
#define BOOST_CSTDFLOAT_FLOAT128_ERF         __erfq
#define BOOST_CSTDFLOAT_FLOAT128_ERFC        __erfcq
#define BOOST_CSTDFLOAT_FLOAT128_LLROUND     __llroundq
#define BOOST_CSTDFLOAT_FLOAT128_LROUND      __lroundq
#define BOOST_CSTDFLOAT_FLOAT128_ROUND       __roundq
#define BOOST_CSTDFLOAT_FLOAT128_NEARBYINT   __nearbyintq
#define BOOST_CSTDFLOAT_FLOAT128_LLRINT      __llrintq
#define BOOST_CSTDFLOAT_FLOAT128_LRINT       __lrintq
#define BOOST_CSTDFLOAT_FLOAT128_RINT        __rintq
#define BOOST_CSTDFLOAT_FLOAT128_MODF        __modfq
#define BOOST_CSTDFLOAT_FLOAT128_SCALBLN     __scalblnq
#define BOOST_CSTDFLOAT_FLOAT128_SCALBN      __scalbnq
#define BOOST_CSTDFLOAT_FLOAT128_ILOGB       __ilogbq
#define BOOST_CSTDFLOAT_FLOAT128_LOGB        __logbq
#define BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER   __nextafterq
//#define BOOST_CSTDFLOAT_FLOAT128_NEXTTOWARD  __nexttowardq
#define BOOST_CSTDFLOAT_FLOAT128_COPYSIGN     __copysignq
#define BOOST_CSTDFLOAT_FLOAT128_SIGNBIT      __signbitq
//#define BOOST_CSTDFLOAT_FLOAT128_FPCLASSIFY __fpclassifyq
//#define BOOST_CSTDFLOAT_FLOAT128_ISFINITE   __isfiniteq
#define BOOST_CSTDFLOAT_FLOAT128_ISINF        __isinfq
#define BOOST_CSTDFLOAT_FLOAT128_ISNAN        __isnanq
//#define BOOST_CSTDFLOAT_FLOAT128_ISNORMAL   __isnormalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISGREATER  __isgreaterq
//#define BOOST_CSTDFLOAT_FLOAT128_ISGREATEREQUAL __isgreaterequalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESS         __islessq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESSEQUAL    __islessequalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESSGREATER  __islessgreaterq
//#define BOOST_CSTDFLOAT_FLOAT128_ISUNORDERED    __isunorderedq
//   end more functions
#elif defined(__GNUC__)
#define BOOST_CSTDFLOAT_FLOAT128_LDEXP  ldexpq
#define BOOST_CSTDFLOAT_FLOAT128_FREXP  frexpq
#define BOOST_CSTDFLOAT_FLOAT128_FABS   fabsq
#define BOOST_CSTDFLOAT_FLOAT128_FLOOR  floorq
#define BOOST_CSTDFLOAT_FLOAT128_CEIL   ceilq
#if !defined(BOOST_CSTDFLOAT_FLOAT128_SQRT)
#define BOOST_CSTDFLOAT_FLOAT128_SQRT   sqrtq
#endif
#define BOOST_CSTDFLOAT_FLOAT128_TRUNC  truncq
#define BOOST_CSTDFLOAT_FLOAT128_POW    powq
#define BOOST_CSTDFLOAT_FLOAT128_LOG    logq
#define BOOST_CSTDFLOAT_FLOAT128_LOG10  log10q
#define BOOST_CSTDFLOAT_FLOAT128_SIN    sinq
#define BOOST_CSTDFLOAT_FLOAT128_COS    cosq
#define BOOST_CSTDFLOAT_FLOAT128_TAN    tanq
#define BOOST_CSTDFLOAT_FLOAT128_ASIN   asinq
#define BOOST_CSTDFLOAT_FLOAT128_ACOS   acosq
#define BOOST_CSTDFLOAT_FLOAT128_ATAN   atanq
#define BOOST_CSTDFLOAT_FLOAT128_FMOD   fmodq
#define BOOST_CSTDFLOAT_FLOAT128_ATAN2  atan2q
#define BOOST_CSTDFLOAT_FLOAT128_LGAMMA lgammaq
#if !defined(BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS)
#define BOOST_CSTDFLOAT_FLOAT128_EXP    expq
#define BOOST_CSTDFLOAT_FLOAT128_EXPM1  expm1q
#define BOOST_CSTDFLOAT_FLOAT128_SINH   sinhq
#define BOOST_CSTDFLOAT_FLOAT128_COSH   coshq
#define BOOST_CSTDFLOAT_FLOAT128_TANH   tanhq
#define BOOST_CSTDFLOAT_FLOAT128_ASINH  asinhq
#define BOOST_CSTDFLOAT_FLOAT128_ACOSH  acoshq
#define BOOST_CSTDFLOAT_FLOAT128_ATANH  atanhq
#define BOOST_CSTDFLOAT_FLOAT128_TGAMMA tgammaq
#else // BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS
#define BOOST_CSTDFLOAT_FLOAT128_EXP    expq_patch
#define BOOST_CSTDFLOAT_FLOAT128_SINH   sinhq_patch
#define BOOST_CSTDFLOAT_FLOAT128_COSH   coshq_patch
#define BOOST_CSTDFLOAT_FLOAT128_TANH   tanhq_patch
#define BOOST_CSTDFLOAT_FLOAT128_ASINH  asinhq_patch
#define BOOST_CSTDFLOAT_FLOAT128_ACOSH  acoshq_patch
#define BOOST_CSTDFLOAT_FLOAT128_ATANH  atanhq_patch
#define BOOST_CSTDFLOAT_FLOAT128_TGAMMA tgammaq_patch
#endif // BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS
//   begin more functions
#define BOOST_CSTDFLOAT_FLOAT128_REMAINDER   remainderq
#define BOOST_CSTDFLOAT_FLOAT128_REMQUO      remquoq
#define BOOST_CSTDFLOAT_FLOAT128_FMA         fmaq
#define BOOST_CSTDFLOAT_FLOAT128_FMAX        fmaxq
#define BOOST_CSTDFLOAT_FLOAT128_FMIN        fminq
#define BOOST_CSTDFLOAT_FLOAT128_FDIM        fdimq
#define BOOST_CSTDFLOAT_FLOAT128_NAN         nanq
//#define BOOST_CSTDFLOAT_FLOAT128_EXP2      exp2q
#define BOOST_CSTDFLOAT_FLOAT128_LOG2        log2q
#define BOOST_CSTDFLOAT_FLOAT128_LOG1P       log1pq
#define BOOST_CSTDFLOAT_FLOAT128_CBRT        cbrtq
#define BOOST_CSTDFLOAT_FLOAT128_HYPOT       hypotq
#define BOOST_CSTDFLOAT_FLOAT128_ERF         erfq
#define BOOST_CSTDFLOAT_FLOAT128_ERFC        erfcq
#define BOOST_CSTDFLOAT_FLOAT128_LLROUND     llroundq
#define BOOST_CSTDFLOAT_FLOAT128_LROUND      lroundq
#define BOOST_CSTDFLOAT_FLOAT128_ROUND       roundq
#define BOOST_CSTDFLOAT_FLOAT128_NEARBYINT   nearbyintq
#define BOOST_CSTDFLOAT_FLOAT128_LLRINT      llrintq
#define BOOST_CSTDFLOAT_FLOAT128_LRINT       lrintq
#define BOOST_CSTDFLOAT_FLOAT128_RINT        rintq
#define BOOST_CSTDFLOAT_FLOAT128_MODF        modfq
#define BOOST_CSTDFLOAT_FLOAT128_SCALBLN     scalblnq
#define BOOST_CSTDFLOAT_FLOAT128_SCALBN      scalbnq
#define BOOST_CSTDFLOAT_FLOAT128_ILOGB       ilogbq
#define BOOST_CSTDFLOAT_FLOAT128_LOGB        logbq
#define BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER   nextafterq
//#define BOOST_CSTDFLOAT_FLOAT128_NEXTTOWARD nexttowardq
#define BOOST_CSTDFLOAT_FLOAT128_COPYSIGN    copysignq
#define BOOST_CSTDFLOAT_FLOAT128_SIGNBIT     signbitq
//#define BOOST_CSTDFLOAT_FLOAT128_FPCLASSIFY fpclassifyq
//#define BOOST_CSTDFLOAT_FLOAT128_ISFINITE   isfiniteq
#define BOOST_CSTDFLOAT_FLOAT128_ISINF        isinfq
#define BOOST_CSTDFLOAT_FLOAT128_ISNAN        isnanq
//#define BOOST_CSTDFLOAT_FLOAT128_ISNORMAL   isnormalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISGREATER  isgreaterq
//#define BOOST_CSTDFLOAT_FLOAT128_ISGREATEREQUAL isgreaterequalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESS         islessq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESSEQUAL    islessequalq
//#define BOOST_CSTDFLOAT_FLOAT128_ISLESSGREATER  islessgreaterq
//#define BOOST_CSTDFLOAT_FLOAT128_ISUNORDERED    isunorderedq
//   end more functions
#endif

// Implement quadruple-precision <cmath> functions in the namespace
// boost::math::cstdfloat::detail. Subsequently inject these into the
// std namespace via *using* directive.

// Begin with some forward function declarations. Also implement patches
// for compilers that have broken float128 exponential functions.

extern "C" int quadmath_snprintf(char*, std::size_t, const char*, ...) throw();

extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_LDEXP(boost::math::cstdfloat::detail::float_internal128_t, int) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_FREXP(boost::math::cstdfloat::detail::float_internal128_t, int*) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_FABS(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_FLOOR(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_CEIL(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_SQRT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TRUNC(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_POW(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_LOG(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_LOG10(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_SIN(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_COS(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TAN(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ASIN(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ACOS(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ATAN(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_FMOD(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ATAN2(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_LGAMMA(boost::math::cstdfloat::detail::float_internal128_t) throw();

//   begin more functions
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_REMAINDER(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_REMQUO(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t, int*) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_FMA(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_FMAX(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_FMIN(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_FDIM(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_NAN(const char*) throw();
//extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_EXP2         (boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_LOG2(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_LOG1P(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_CBRT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_HYPOT(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_ERF(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_ERFC(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" long long int                                BOOST_CSTDFLOAT_FLOAT128_LLROUND(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" long int                                   BOOST_CSTDFLOAT_FLOAT128_LROUND(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_ROUND(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_NEARBYINT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" long long int                                BOOST_CSTDFLOAT_FLOAT128_LLRINT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" long int                                   BOOST_CSTDFLOAT_FLOAT128_LRINT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_RINT(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_MODF(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t*) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_SCALBLN(boost::math::cstdfloat::detail::float_internal128_t, long int) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_SCALBN(boost::math::cstdfloat::detail::float_internal128_t, int) throw();
extern "C" int                                      BOOST_CSTDFLOAT_FLOAT128_ILOGB(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_LOGB(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_NEXTTOWARD   (boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_COPYSIGN(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" int                                                  BOOST_CSTDFLOAT_FLOAT128_SIGNBIT(boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_FPCLASSIFY   (boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISFINITE      (boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" int                                                  BOOST_CSTDFLOAT_FLOAT128_ISINF(boost::math::cstdfloat::detail::float_internal128_t) throw();
extern "C" int                                                  BOOST_CSTDFLOAT_FLOAT128_ISNAN(boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" boost::math::cstdfloat::detail::float_internal128_t  BOOST_CSTDFLOAT_FLOAT128_ISNORMAL   (boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISGREATER   (boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISGREATEREQUAL(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISLESS      (boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISLESSEQUAL   (boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISLESSGREATER(boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
//extern "C" int                                                BOOST_CSTDFLOAT_FLOAT128_ISUNORDERED   (boost::math::cstdfloat::detail::float_internal128_t, boost::math::cstdfloat::detail::float_internal128_t) throw();
 //   end more functions

#if !defined(BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS)

extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_EXP(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_EXPM1(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_SINH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_COSH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TANH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ASINH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ACOSH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ATANH(boost::math::cstdfloat::detail::float_internal128_t x) throw();
extern "C" boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TGAMMA(boost::math::cstdfloat::detail::float_internal128_t x) throw();
 
#else // BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS

// Forward declaration of the patched exponent function, exp(x).
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_EXP(boost::math::cstdfloat::detail::float_internal128_t x);

inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_EXPM1(boost::math::cstdfloat::detail::float_internal128_t x)
{
   // Compute exp(x) - 1 for x small.

   // Use an order-36 polynomial approximation of the exponential function
   // in the range of (-ln2 < x < ln2). Scale the argument to this range
   // and subsequently multiply the result by 2^n accordingly.

   // Derive the polynomial coefficients with Mathematica(R) by generating
   // a table of high-precision values of exp(x) in the range (-ln2 < x < ln2)
   // and subsequently applying the built-in *Fit* function.

   // Table[{x, Exp[x] - 1}, {x, -Log[2], Log[2], 1/180}]
   // N[%, 120]
   // Fit[%, {x, x^2, x^3, x^4, x^5, x^6, x^7, x^8, x^9, x^10, x^11, x^12,
   //         x^13, x^14, x^15, x^16, x^17, x^18, x^19, x^20, x^21, x^22,
   //         x^23, x^24, x^25, x^26, x^27, x^28, x^29, x^30, x^31, x^32,
   //         x^33, x^34, x^35, x^36}, x]

   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;

   float_type sum;

   if (x > BOOST_FLOAT128_C(0.693147180559945309417232121458176568075500134360255))
   {
      sum = ::BOOST_CSTDFLOAT_FLOAT128_EXP(x) - float_type(1);
   }
   else
   {
      // Compute the polynomial approximation of exp(alpha).
      sum = ((((((((((((((((((((((((((((((((((((float_type(BOOST_FLOAT128_C(2.69291698127774166063293705964720493864630783729857438187365E-42))  * x
         + float_type(BOOST_FLOAT128_C(9.70937085471487654794114679403710456028986572118859594614033E-41))) * x
         + float_type(BOOST_FLOAT128_C(3.38715585158055097155585505318085512156885389014410753080500E-39))) * x
         + float_type(BOOST_FLOAT128_C(1.15162718532861050809222658798662695267019717760563645440433E-37))) * x
         + float_type(BOOST_FLOAT128_C(3.80039074689434663295873584133017767349635602413675471702393E-36))) * x
         + float_type(BOOST_FLOAT128_C(1.21612504934087520075905434734158045947460467096773246215239E-34))) * x
         + float_type(BOOST_FLOAT128_C(3.76998762883139753126119821241037824830069851253295480396224E-33))) * x
         + float_type(BOOST_FLOAT128_C(1.13099628863830344684998293828608215735777107850991029729440E-31))) * x
         + float_type(BOOST_FLOAT128_C(3.27988923706982293204067897468714277771890104022419696770352E-30))) * x
         + float_type(BOOST_FLOAT128_C(9.18368986379558482800593745627556950089950023355628325088207E-29))) * x
         + float_type(BOOST_FLOAT128_C(2.47959626322479746949155352659617642905315302382639380521497E-27))) * x
         + float_type(BOOST_FLOAT128_C(6.44695028438447337900255966737803112935639344283098705091949E-26))) * x
         + float_type(BOOST_FLOAT128_C(1.61173757109611834904452725462599961406036904573072897122957E-24))) * x
         + float_type(BOOST_FLOAT128_C(3.86817017063068403772269360016918092488847584660382953555804E-23))) * x
         + float_type(BOOST_FLOAT128_C(8.89679139245057328674891109315654704307721758924206107351744E-22))) * x
         + float_type(BOOST_FLOAT128_C(1.95729410633912612308475595397946731738088422488032228717097E-20))) * x
         + float_type(BOOST_FLOAT128_C(4.11031762331216485847799061511674191805055663711439605760231E-19))) * x
         + float_type(BOOST_FLOAT128_C(8.22063524662432971695598123977873600603370758794431071426640E-18))) * x
         + float_type(BOOST_FLOAT128_C(1.56192069685862264622163643500633782667263448653185159383285E-16))) * x
         + float_type(BOOST_FLOAT128_C(2.81145725434552076319894558300988749849555291507956994126835E-15))) * x
         + float_type(BOOST_FLOAT128_C(4.77947733238738529743820749111754320727153728139716409114011E-14))) * x
         + float_type(BOOST_FLOAT128_C(7.64716373181981647590113198578807092707697416852226691068627E-13))) * x
         + float_type(BOOST_FLOAT128_C(1.14707455977297247138516979786821056670509688396295740818677E-11))) * x
         + float_type(BOOST_FLOAT128_C(1.60590438368216145993923771701549479323291461578567184216302E-10))) * x
         + float_type(BOOST_FLOAT128_C(2.08767569878680989792100903212014323125428376052986408239620E-09))) * x
         + float_type(BOOST_FLOAT128_C(2.50521083854417187750521083854417187750523408006206780016659E-08))) * x
         + float_type(BOOST_FLOAT128_C(2.75573192239858906525573192239858906525573195144226062684604E-07))) * x
         + float_type(BOOST_FLOAT128_C(2.75573192239858906525573192239858906525573191310049321957902E-06))) * x
         + float_type(BOOST_FLOAT128_C(0.00002480158730158730158730158730158730158730158730149317774)))     * x
         + float_type(BOOST_FLOAT128_C(0.00019841269841269841269841269841269841269841269841293575920)))     * x
         + float_type(BOOST_FLOAT128_C(0.00138888888888888888888888888888888888888888888888889071045)))     * x
         + float_type(BOOST_FLOAT128_C(0.00833333333333333333333333333333333333333333333333332986595)))     * x
         + float_type(BOOST_FLOAT128_C(0.04166666666666666666666666666666666666666666666666666664876)))     * x
         + float_type(BOOST_FLOAT128_C(0.16666666666666666666666666666666666666666666666666666669048)))     * x
         + float_type(BOOST_FLOAT128_C(0.50000000000000000000000000000000000000000000000000000000006)))     * x
         + float_type(BOOST_FLOAT128_C(0.99999999999999999999999999999999999999999999999999999999995)))     * x);
   }

   return sum;
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_EXP(boost::math::cstdfloat::detail::float_internal128_t x)
{
   // Patch the expq() function for a subset of broken GCC compilers
   // like GCC 4.7, 4.8 on MinGW.

   // Use an order-36 polynomial approximation of the exponential function
   // in the range of (-ln2 < x < ln2). Scale the argument to this range
   // and subsequently multiply the result by 2^n accordingly.

   // Derive the polynomial coefficients with Mathematica(R) by generating
   // a table of high-precision values of exp(x) in the range (-ln2 < x < ln2)
   // and subsequently applying the built-in *Fit* function.

   // Table[{x, Exp[x] - 1}, {x, -Log[2], Log[2], 1/180}]
   // N[%, 120]
   // Fit[%, {x, x^2, x^3, x^4, x^5, x^6, x^7, x^8, x^9, x^10, x^11, x^12,
   //         x^13, x^14, x^15, x^16, x^17, x^18, x^19, x^20, x^21, x^22,
   //         x^23, x^24, x^25, x^26, x^27, x^28, x^29, x^30, x^31, x^32,
   //         x^33, x^34, x^35, x^36}, x]

   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;

   // Scale the argument x to the range (-ln2 < x < ln2).
   constexpr float_type one_over_ln2 = float_type(BOOST_FLOAT128_C(1.44269504088896340735992468100189213742664595415299));
   const float_type x_over_ln2 = x * one_over_ln2;

   std::int_fast32_t n;

   if (x != x)
   {
      // The argument is NaN.
      return std::numeric_limits<float_type>::quiet_NaN();
   }
   else if (::BOOST_CSTDFLOAT_FLOAT128_FABS(x) > BOOST_FLOAT128_C(+0.693147180559945309417232121458176568075500134360255))
   {
      // The absolute value of the argument exceeds ln2.
      n = static_cast<std::int_fast32_t>(::BOOST_CSTDFLOAT_FLOAT128_FLOOR(x_over_ln2));
   }
   else if (::BOOST_CSTDFLOAT_FLOAT128_FABS(x) < BOOST_FLOAT128_C(+0.693147180559945309417232121458176568075500134360255))
   {
      // The absolute value of the argument is less than ln2.
      n = static_cast<std::int_fast32_t>(0);
   }
   else
   {
      // The absolute value of the argument is exactly equal to ln2 (in the sense of floating-point equality).
      return float_type(2);
   }

   // Check if the argument is very near an integer.
   const float_type floor_of_x = ::BOOST_CSTDFLOAT_FLOAT128_FLOOR(x);

   if (::BOOST_CSTDFLOAT_FLOAT128_FABS(x - floor_of_x) < float_type(BOOST_CSTDFLOAT_FLOAT128_EPS))
   {
      // Return e^n for arguments very near an integer.
      return boost::math::cstdfloat::detail::pown(BOOST_FLOAT128_C(2.71828182845904523536028747135266249775724709369996), static_cast<std::int_fast32_t>(floor_of_x));
   }

   // Compute the scaled argument alpha.
   const float_type alpha = x - (n * BOOST_FLOAT128_C(0.693147180559945309417232121458176568075500134360255));

   // Compute the polynomial approximation of expm1(alpha) and add to it
   // in order to obtain the scaled result.
   const float_type scaled_result = ::BOOST_CSTDFLOAT_FLOAT128_EXPM1(alpha) + float_type(1);

   // Rescale the result and return it.
   return scaled_result * boost::math::cstdfloat::detail::pown(float_type(2), n);
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_SINH(boost::math::cstdfloat::detail::float_internal128_t x)
{
   // Patch the sinhq() function for a subset of broken GCC compilers
   // like GCC 4.7, 4.8 on MinGW.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;

   // Here, we use the following:
   // Set: ex  = exp(x)
   // Set: em1 = expm1(x)
   // Then
   // sinh(x) = (ex - 1/ex) / 2         ; for |x| >= 1
   // sinh(x) = (2em1 + em1^2) / (2ex)  ; for |x| < 1

   const float_type ex = ::BOOST_CSTDFLOAT_FLOAT128_EXP(x);

   if (::BOOST_CSTDFLOAT_FLOAT128_FABS(x) < float_type(+1))
   {
      const float_type em1 = ::BOOST_CSTDFLOAT_FLOAT128_EXPM1(x);

      return ((em1 * 2) + (em1 * em1)) / (ex * 2);
   }
   else
   {
      return (ex - (float_type(1) / ex)) / 2;
   }
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_COSH(boost::math::cstdfloat::detail::float_internal128_t x)
{
   // Patch the coshq() function for a subset of broken GCC compilers
   // like GCC 4.7, 4.8 on MinGW.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;
   const float_type ex = ::BOOST_CSTDFLOAT_FLOAT128_EXP(x);
   return (ex + (float_type(1) / ex)) / 2;
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TANH(boost::math::cstdfloat::detail::float_internal128_t x)
{
   // Patch the tanhq() function for a subset of broken GCC compilers
   // like GCC 4.7, 4.8 on MinGW.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;
   const float_type ex_plus = ::BOOST_CSTDFLOAT_FLOAT128_EXP(x);
   const float_type ex_minus = (float_type(1) / ex_plus);
   return (ex_plus - ex_minus) / (ex_plus + ex_minus);
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ASINH(boost::math::cstdfloat::detail::float_internal128_t x) throw()
{
   // Patch the asinh() function since quadmath does not have it.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;
   return ::BOOST_CSTDFLOAT_FLOAT128_LOG(x + ::BOOST_CSTDFLOAT_FLOAT128_SQRT((x * x) + float_type(1)));
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ACOSH(boost::math::cstdfloat::detail::float_internal128_t x) throw()
{
   // Patch the acosh() function since quadmath does not have it.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;
   const float_type zp(x + float_type(1));
   const float_type zm(x - float_type(1));

   return ::BOOST_CSTDFLOAT_FLOAT128_LOG(x + (zp * ::BOOST_CSTDFLOAT_FLOAT128_SQRT(zm / zp)));
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_ATANH(boost::math::cstdfloat::detail::float_internal128_t x) throw()
{
   // Patch the atanh() function since quadmath does not have it.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;
   return (::BOOST_CSTDFLOAT_FLOAT128_LOG(float_type(1) + x)
      - ::BOOST_CSTDFLOAT_FLOAT128_LOG(float_type(1) - x)) / 2;
}
inline boost::math::cstdfloat::detail::float_internal128_t BOOST_CSTDFLOAT_FLOAT128_TGAMMA(boost::math::cstdfloat::detail::float_internal128_t x) throw()
{
   // Patch the tgammaq() function for a subset of broken GCC compilers
   // like GCC 4.7, 4.8 on MinGW.
   typedef boost::math::cstdfloat::detail::float_internal128_t float_type;

   if (x > float_type(0))
   {
      return ::BOOST_CSTDFLOAT_FLOAT128_EXP(::BOOST_CSTDFLOAT_FLOAT128_LGAMMA(x));
   }
   else if (x < float_type(0))
   {
      // For x < 0, compute tgamma(-x) and use the reflection formula.
      const float_type positive_x = -x;
      float_type gamma_value = ::BOOST_CSTDFLOAT_FLOAT128_TGAMMA(positive_x);
      const float_type floor_of_positive_x = ::BOOST_CSTDFLOAT_FLOAT128_FLOOR(positive_x);

      // Take the reflection checks (slightly adapted) from <boost/math/gamma.hpp>.
      const bool floor_of_z_is_equal_to_z = (positive_x == ::BOOST_CSTDFLOAT_FLOAT128_FLOOR(positive_x));

      constexpr float_type my_pi = BOOST_FLOAT128_C(3.14159265358979323846264338327950288419716939937511);

      if (floor_of_z_is_equal_to_z)
      {
         const bool is_odd = ((std::int32_t(floor_of_positive_x) % std::int32_t(2)) != std::int32_t(0));

         return (is_odd ? -std::numeric_limits<float_type>::infinity()
            : +std::numeric_limits<float_type>::infinity());
      }

      const float_type sinpx_value = x * ::BOOST_CSTDFLOAT_FLOAT128_SIN(my_pi * x);

      gamma_value *= sinpx_value;

      const bool result_is_too_large_to_represent = ((::BOOST_CSTDFLOAT_FLOAT128_FABS(gamma_value) < float_type(1))
         && (((std::numeric_limits<float_type>::max)() * ::BOOST_CSTDFLOAT_FLOAT128_FABS(gamma_value)) < my_pi));

      if (result_is_too_large_to_represent)
      {
         const bool is_odd = ((std::int32_t(floor_of_positive_x) % std::int32_t(2)) != std::int32_t(0));

         return (is_odd ? -std::numeric_limits<float_type>::infinity()
            : +std::numeric_limits<float_type>::infinity());
      }

      gamma_value = -my_pi / gamma_value;

      if ((gamma_value > float_type(0)) || (gamma_value < float_type(0)))
      {
         return gamma_value;
      }
      else
      {
         // The value of gamma is too small to represent. Return 0.0 here.
         return float_type(0);
      }
   }
   else
   {
      // Gamma of zero is complex infinity. Return NaN here.
      return std::numeric_limits<float_type>::quiet_NaN();
   }
}
#endif // BOOST_CSTDFLOAT_BROKEN_FLOAT128_MATH_FUNCTIONS

// Define the quadruple-precision <cmath> functions in the namespace boost::math::cstdfloat::detail.

namespace boost {
   namespace math {
      namespace cstdfloat {
         namespace detail {
            inline   boost::math::cstdfloat::detail::float_internal128_t ldexp(boost::math::cstdfloat::detail::float_internal128_t x, int n) { return ::BOOST_CSTDFLOAT_FLOAT128_LDEXP(x, n); }
            inline   boost::math::cstdfloat::detail::float_internal128_t frexp(boost::math::cstdfloat::detail::float_internal128_t x, int* pn) { return ::BOOST_CSTDFLOAT_FLOAT128_FREXP(x, pn); }
            inline   boost::math::cstdfloat::detail::float_internal128_t fabs(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_FABS(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t abs(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_FABS(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t floor(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_FLOOR(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t ceil(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_CEIL(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t sqrt(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_SQRT(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t trunc(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_TRUNC(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t exp(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_EXP(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t expm1(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_EXPM1(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t pow(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t a) { return ::BOOST_CSTDFLOAT_FLOAT128_POW(x, a); }
            inline   boost::math::cstdfloat::detail::float_internal128_t pow(boost::math::cstdfloat::detail::float_internal128_t x, int a) { return ::BOOST_CSTDFLOAT_FLOAT128_POW(x, boost::math::cstdfloat::detail::float_internal128_t(a)); }
            inline   boost::math::cstdfloat::detail::float_internal128_t log(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LOG(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t log10(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LOG10(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t sin(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_SIN(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t cos(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_COS(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t tan(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_TAN(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t asin(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ASIN(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t acos(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ACOS(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t atan(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ATAN(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t sinh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_SINH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t cosh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_COSH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t tanh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_TANH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t asinh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ASINH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t acosh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ACOSH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t atanh(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ATANH(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t fmod(boost::math::cstdfloat::detail::float_internal128_t a, boost::math::cstdfloat::detail::float_internal128_t b) { return ::BOOST_CSTDFLOAT_FLOAT128_FMOD(a, b); }
            inline   boost::math::cstdfloat::detail::float_internal128_t atan2(boost::math::cstdfloat::detail::float_internal128_t y, boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ATAN2(y, x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t lgamma(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LGAMMA(x); }
            inline   boost::math::cstdfloat::detail::float_internal128_t tgamma(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_TGAMMA(x); }
            //   begin more functions
            inline boost::math::cstdfloat::detail::float_internal128_t  remainder(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_REMAINDER(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  remquo(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y, int* z) { return ::BOOST_CSTDFLOAT_FLOAT128_REMQUO(x, y, z); }
            inline boost::math::cstdfloat::detail::float_internal128_t  fma(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y, boost::math::cstdfloat::detail::float_internal128_t z) { return BOOST_CSTDFLOAT_FLOAT128_FMA(x, y, z); }

            inline boost::math::cstdfloat::detail::float_internal128_t  fmax(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMAX(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               fmax(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMAX(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               fmax(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMAX(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  fmin(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMIN(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               fmin(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMIN(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               fmin(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_FMIN(x, y); }

            inline boost::math::cstdfloat::detail::float_internal128_t  fdim(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_FDIM(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  nanq(const char* x) { return ::BOOST_CSTDFLOAT_FLOAT128_NAN(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  exp2(boost::math::cstdfloat::detail::float_internal128_t x)
            {
               return ::BOOST_CSTDFLOAT_FLOAT128_POW(boost::math::cstdfloat::detail::float_internal128_t(2), x);
            }
            inline boost::math::cstdfloat::detail::float_internal128_t  log2(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LOG2(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  log1p(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LOG1P(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  cbrt(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_CBRT(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  hypot(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y, boost::math::cstdfloat::detail::float_internal128_t z) { return ::BOOST_CSTDFLOAT_FLOAT128_SQRT(x*x + y * y + z * z); }
            inline boost::math::cstdfloat::detail::float_internal128_t  hypot(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_HYPOT(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               hypot(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return ::BOOST_CSTDFLOAT_FLOAT128_HYPOT(x, y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               hypot(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_HYPOT(x, y); }


            inline boost::math::cstdfloat::detail::float_internal128_t  erf(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ERF(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  erfc(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ERFC(x); }
            inline long long int                                        llround(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LLROUND(x); }
            inline long int                                             lround(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LROUND(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  round(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ROUND(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  nearbyint(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_NEARBYINT(x); }
            inline long long int                                        llrint(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LLRINT(x); }
            inline long int                                             lrint(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LRINT(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  rint(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_RINT(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  modf(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t* y) { return ::BOOST_CSTDFLOAT_FLOAT128_MODF(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  scalbln(boost::math::cstdfloat::detail::float_internal128_t x, long int y) { return ::BOOST_CSTDFLOAT_FLOAT128_SCALBLN(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  scalbn(boost::math::cstdfloat::detail::float_internal128_t x, int y) { return ::BOOST_CSTDFLOAT_FLOAT128_SCALBN(x, y); }
            inline int                                                  ilogb(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ILOGB(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  logb(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_LOGB(x); }
            inline boost::math::cstdfloat::detail::float_internal128_t  nextafter(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER(x, y); }
            inline boost::math::cstdfloat::detail::float_internal128_t  nexttoward(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return -(::BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER(-x, -y)); }
            inline boost::math::cstdfloat::detail::float_internal128_t  copysign   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_COPYSIGN(x, y); }
            inline bool                                                 signbit   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_SIGNBIT(x); }
            inline int                                                  fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x)
            {
               if (::BOOST_CSTDFLOAT_FLOAT128_ISNAN(x))
                  return FP_NAN;
               else if (::BOOST_CSTDFLOAT_FLOAT128_ISINF(x))
                  return FP_INFINITE;
               else if (x == BOOST_FLOAT128_C(0.0))
                  return FP_ZERO;

               if (::BOOST_CSTDFLOAT_FLOAT128_FABS(x) < BOOST_CSTDFLOAT_FLOAT128_MIN)
                  return FP_SUBNORMAL;
               else
                  return FP_NORMAL;
            }
            inline bool                                      isfinite   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x)
            {
               return !::BOOST_CSTDFLOAT_FLOAT128_ISNAN(x) && !::BOOST_CSTDFLOAT_FLOAT128_ISINF(x);
            }
            inline bool                                      isinf      BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ISINF(x); }
            inline bool                                      isnan      BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x) { return ::BOOST_CSTDFLOAT_FLOAT128_ISNAN(x); }
            inline bool                                      isnormal   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x) { return boost::math::cstdfloat::detail::fpclassify BOOST_PREVENT_MACRO_SUBSTITUTION(x) == FP_NORMAL; }
            inline bool                                      isgreater      BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y)
            {
               if (isnan BOOST_PREVENT_MACRO_SUBSTITUTION(x) || isnan BOOST_PREVENT_MACRO_SUBSTITUTION(y))
                  return false;
               return x > y;
            }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isgreater BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return isgreater BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isgreater BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return isgreater BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }

            inline bool                                      isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y)
            {
               if (isnan BOOST_PREVENT_MACRO_SUBSTITUTION(x) || isnan BOOST_PREVENT_MACRO_SUBSTITUTION(y))
                  return false;
               return x >= y;
            }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return isgreaterequal BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }

            inline bool                                      isless      BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y)
            {
               if (isnan BOOST_PREVENT_MACRO_SUBSTITUTION(x) || isnan BOOST_PREVENT_MACRO_SUBSTITUTION(y))
                  return false;
               return x < y;
            }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isless BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return isless BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isless BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return isless BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }


            inline bool                                      islessequal   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y)
            {
               if (isnan BOOST_PREVENT_MACRO_SUBSTITUTION(x) || isnan BOOST_PREVENT_MACRO_SUBSTITUTION(y))
                  return false;
               return x <= y;
            }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               islessequal BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return islessequal BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               islessequal BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return islessequal BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }


            inline bool                                      islessgreater   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y)
            {
               if (isnan BOOST_PREVENT_MACRO_SUBSTITUTION(x) || isnan BOOST_PREVENT_MACRO_SUBSTITUTION(y))
                  return false;
               return (x < y) || (x > y);
            }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return islessgreater BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }


            inline bool                                      isunordered   BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, boost::math::cstdfloat::detail::float_internal128_t y) { return ::BOOST_CSTDFLOAT_FLOAT128_ISNAN(x) || ::BOOST_CSTDFLOAT_FLOAT128_ISNAN(y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isunordered BOOST_PREVENT_MACRO_SUBSTITUTION(boost::math::cstdfloat::detail::float_internal128_t x, T y) { return isunordered BOOST_PREVENT_MACRO_SUBSTITUTION(x, (boost::math::cstdfloat::detail::float_internal128_t)y); }
            template <class T>
            inline typename std::enable_if<
               std::is_convertible<T, boost::math::cstdfloat::detail::float_internal128_t>::value
               && !std::is_same<T, boost::math::cstdfloat::detail::float_internal128_t>::value, boost::math::cstdfloat::detail::float_internal128_t>::type
               isunordered BOOST_PREVENT_MACRO_SUBSTITUTION(T x, boost::math::cstdfloat::detail::float_internal128_t y) { return isunordered BOOST_PREVENT_MACRO_SUBSTITUTION((boost::math::cstdfloat::detail::float_internal128_t)x, y); }


            //   end more functions
         }
      }
   }
} // boost::math::cstdfloat::detail

// We will now inject the quadruple-precision <cmath> functions
// into the std namespace. This is done via *using* directive.
namespace std
{
   using boost::math::cstdfloat::detail::ldexp;
   using boost::math::cstdfloat::detail::frexp;
   using boost::math::cstdfloat::detail::fabs;

#if !(defined(_GLIBCXX_USE_FLOAT128) && defined(__GNUC__) && (__GNUC__ >= 7))
#if (defined(__clang__) && !(!defined(__STRICT_ANSI__) && defined(_GLIBCXX_USE_FLOAT128))) || (__GNUC__ <= 6 && !defined(__clang__)) 
   // workaround for clang using libstdc++ and old GCC
   using boost::math::cstdfloat::detail::abs;
#endif
#endif

   using boost::math::cstdfloat::detail::floor;
   using boost::math::cstdfloat::detail::ceil;
   using boost::math::cstdfloat::detail::sqrt;
   using boost::math::cstdfloat::detail::trunc;
   using boost::math::cstdfloat::detail::exp;
   using boost::math::cstdfloat::detail::expm1;
   using boost::math::cstdfloat::detail::pow;
   using boost::math::cstdfloat::detail::log;
   using boost::math::cstdfloat::detail::log10;
   using boost::math::cstdfloat::detail::sin;
   using boost::math::cstdfloat::detail::cos;
   using boost::math::cstdfloat::detail::tan;
   using boost::math::cstdfloat::detail::asin;
   using boost::math::cstdfloat::detail::acos;
   using boost::math::cstdfloat::detail::atan;
   using boost::math::cstdfloat::detail::sinh;
   using boost::math::cstdfloat::detail::cosh;
   using boost::math::cstdfloat::detail::tanh;
   using boost::math::cstdfloat::detail::asinh;
   using boost::math::cstdfloat::detail::acosh;
   using boost::math::cstdfloat::detail::atanh;
   using boost::math::cstdfloat::detail::fmod;
   using boost::math::cstdfloat::detail::atan2;
   using boost::math::cstdfloat::detail::lgamma;
   using boost::math::cstdfloat::detail::tgamma;

   //   begin more functions
   using boost::math::cstdfloat::detail::remainder;
   using boost::math::cstdfloat::detail::remquo;
   using boost::math::cstdfloat::detail::fma;
   using boost::math::cstdfloat::detail::fmax;
   using boost::math::cstdfloat::detail::fmin;
   using boost::math::cstdfloat::detail::fdim;
   using boost::math::cstdfloat::detail::nanq;
   using boost::math::cstdfloat::detail::exp2;
   using boost::math::cstdfloat::detail::log2;
   using boost::math::cstdfloat::detail::log1p;
   using boost::math::cstdfloat::detail::cbrt;
   using boost::math::cstdfloat::detail::hypot;
   using boost::math::cstdfloat::detail::erf;
   using boost::math::cstdfloat::detail::erfc;
   using boost::math::cstdfloat::detail::llround;
   using boost::math::cstdfloat::detail::lround;
   using boost::math::cstdfloat::detail::round;
   using boost::math::cstdfloat::detail::nearbyint;
   using boost::math::cstdfloat::detail::llrint;
   using boost::math::cstdfloat::detail::lrint;
   using boost::math::cstdfloat::detail::rint;
   using boost::math::cstdfloat::detail::modf;
   using boost::math::cstdfloat::detail::scalbln;
   using boost::math::cstdfloat::detail::scalbn;
   using boost::math::cstdfloat::detail::ilogb;
   using boost::math::cstdfloat::detail::logb;
   using boost::math::cstdfloat::detail::nextafter;
   using boost::math::cstdfloat::detail::nexttoward;
   using boost::math::cstdfloat::detail::copysign;
   using boost::math::cstdfloat::detail::signbit;
   using boost::math::cstdfloat::detail::fpclassify;
   using boost::math::cstdfloat::detail::isfinite;
   using boost::math::cstdfloat::detail::isinf;
   using boost::math::cstdfloat::detail::isnan;
   using boost::math::cstdfloat::detail::isnormal;
   using boost::math::cstdfloat::detail::isgreater;
   using boost::math::cstdfloat::detail::isgreaterequal;
   using boost::math::cstdfloat::detail::isless;
   using boost::math::cstdfloat::detail::islessequal;
   using boost::math::cstdfloat::detail::islessgreater;
   using boost::math::cstdfloat::detail::isunordered;
   //   end more functions

   //
   // Very basic iostream operator:
   //
   inline std::ostream& operator << (std::ostream& os, __float128 m_value)
   {
      std::streamsize digits = os.precision();
      std::ios_base::fmtflags f = os.flags();
      std::string s;

      char buf[100];
      std::unique_ptr<char[]> buf2;
      std::string format = "%";
      if (f & std::ios_base::showpos)
         format += "+";
      if (f & std::ios_base::showpoint)
         format += "#";
      format += ".*";
      if (digits == 0)
         digits = 36;
      format += "Q";
      if (f & std::ios_base::scientific)
         format += "e";
      else if (f & std::ios_base::fixed)
         format += "f";
      else
         format += "g";

      int v = quadmath_snprintf(buf, 100, format.c_str(), digits, m_value);

      if ((v < 0) || (v >= 99))
      {
         int v_max = v;
         buf2.reset(new char[v + 3]);
         v = quadmath_snprintf(&buf2[0], v_max + 3, format.c_str(), digits, m_value);
         if (v >= v_max + 3)
         {
            BOOST_MATH_THROW_EXCEPTION(std::runtime_error("Formatting of float128_type failed."));
         }
         s = &buf2[0];
      }
      else
         s = buf;
      std::streamsize ss = os.width();
      if (ss > static_cast<std::streamsize>(s.size()))
      {
         char fill = os.fill();
         if ((os.flags() & std::ios_base::left) == std::ios_base::left)
            s.append(static_cast<std::string::size_type>(ss - s.size()), fill);
         else
            s.insert(static_cast<std::string::size_type>(0), static_cast<std::string::size_type>(ss - s.size()), fill);
      }

      return os << s;
   }


} // namespace std

// We will now remove the preprocessor symbols representing quadruple-precision <cmath>
// functions from the preprocessor.

#undef BOOST_CSTDFLOAT_FLOAT128_LDEXP
#undef BOOST_CSTDFLOAT_FLOAT128_FREXP
#undef BOOST_CSTDFLOAT_FLOAT128_FABS
#undef BOOST_CSTDFLOAT_FLOAT128_FLOOR
#undef BOOST_CSTDFLOAT_FLOAT128_CEIL
#undef BOOST_CSTDFLOAT_FLOAT128_SQRT
#undef BOOST_CSTDFLOAT_FLOAT128_TRUNC
#undef BOOST_CSTDFLOAT_FLOAT128_EXP
#undef BOOST_CSTDFLOAT_FLOAT128_EXPM1
#undef BOOST_CSTDFLOAT_FLOAT128_POW
#undef BOOST_CSTDFLOAT_FLOAT128_LOG
#undef BOOST_CSTDFLOAT_FLOAT128_LOG10
#undef BOOST_CSTDFLOAT_FLOAT128_SIN
#undef BOOST_CSTDFLOAT_FLOAT128_COS
#undef BOOST_CSTDFLOAT_FLOAT128_TAN
#undef BOOST_CSTDFLOAT_FLOAT128_ASIN
#undef BOOST_CSTDFLOAT_FLOAT128_ACOS
#undef BOOST_CSTDFLOAT_FLOAT128_ATAN
#undef BOOST_CSTDFLOAT_FLOAT128_SINH
#undef BOOST_CSTDFLOAT_FLOAT128_COSH
#undef BOOST_CSTDFLOAT_FLOAT128_TANH
#undef BOOST_CSTDFLOAT_FLOAT128_ASINH
#undef BOOST_CSTDFLOAT_FLOAT128_ACOSH
#undef BOOST_CSTDFLOAT_FLOAT128_ATANH
#undef BOOST_CSTDFLOAT_FLOAT128_FMOD
#undef BOOST_CSTDFLOAT_FLOAT128_ATAN2
#undef BOOST_CSTDFLOAT_FLOAT128_LGAMMA
#undef BOOST_CSTDFLOAT_FLOAT128_TGAMMA

//   begin more functions
#undef BOOST_CSTDFLOAT_FLOAT128_REMAINDER
#undef BOOST_CSTDFLOAT_FLOAT128_REMQUO
#undef BOOST_CSTDFLOAT_FLOAT128_FMA
#undef BOOST_CSTDFLOAT_FLOAT128_FMAX
#undef BOOST_CSTDFLOAT_FLOAT128_FMIN
#undef BOOST_CSTDFLOAT_FLOAT128_FDIM
#undef BOOST_CSTDFLOAT_FLOAT128_NAN
#undef BOOST_CSTDFLOAT_FLOAT128_EXP2
#undef BOOST_CSTDFLOAT_FLOAT128_LOG2
#undef BOOST_CSTDFLOAT_FLOAT128_LOG1P
#undef BOOST_CSTDFLOAT_FLOAT128_CBRT
#undef BOOST_CSTDFLOAT_FLOAT128_HYPOT
#undef BOOST_CSTDFLOAT_FLOAT128_ERF
#undef BOOST_CSTDFLOAT_FLOAT128_ERFC
#undef BOOST_CSTDFLOAT_FLOAT128_LLROUND
#undef BOOST_CSTDFLOAT_FLOAT128_LROUND
#undef BOOST_CSTDFLOAT_FLOAT128_ROUND
#undef BOOST_CSTDFLOAT_FLOAT128_NEARBYINT
#undef BOOST_CSTDFLOAT_FLOAT128_LLRINT
#undef BOOST_CSTDFLOAT_FLOAT128_LRINT
#undef BOOST_CSTDFLOAT_FLOAT128_RINT
#undef BOOST_CSTDFLOAT_FLOAT128_MODF
#undef BOOST_CSTDFLOAT_FLOAT128_SCALBLN
#undef BOOST_CSTDFLOAT_FLOAT128_SCALBN
#undef BOOST_CSTDFLOAT_FLOAT128_ILOGB
#undef BOOST_CSTDFLOAT_FLOAT128_LOGB
#undef BOOST_CSTDFLOAT_FLOAT128_NEXTAFTER
#undef BOOST_CSTDFLOAT_FLOAT128_NEXTTOWARD
#undef BOOST_CSTDFLOAT_FLOAT128_COPYSIGN
#undef BOOST_CSTDFLOAT_FLOAT128_SIGNBIT
#undef BOOST_CSTDFLOAT_FLOAT128_FPCLASSIFY
#undef BOOST_CSTDFLOAT_FLOAT128_ISFINITE
#undef BOOST_CSTDFLOAT_FLOAT128_ISINF
#undef BOOST_CSTDFLOAT_FLOAT128_ISNAN
#undef BOOST_CSTDFLOAT_FLOAT128_ISNORMAL
#undef BOOST_CSTDFLOAT_FLOAT128_ISGREATER
#undef BOOST_CSTDFLOAT_FLOAT128_ISGREATEREQUAL
#undef BOOST_CSTDFLOAT_FLOAT128_ISLESS
#undef BOOST_CSTDFLOAT_FLOAT128_ISLESSEQUAL
#undef BOOST_CSTDFLOAT_FLOAT128_ISLESSGREATER
#undef BOOST_CSTDFLOAT_FLOAT128_ISUNORDERED
//   end more functions

#endif // Not BOOST_CSTDFLOAT_NO_LIBQUADMATH_SUPPORT (i.e., the user would like to have libquadmath support)

#endif // BOOST_MATH_CSTDFLOAT_CMATH_2014_02_15_HPP_

