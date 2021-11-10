//  Copyright (c) 2006-7 John Maddock
//  Copyright (c) 2021 Matt Borland
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_TOOLS_CONFIG_HPP
#define BOOST_MATH_TOOLS_CONFIG_HPP

#ifdef _MSC_VER
#pragma once
#endif

#include <boost/math/tools/is_standalone.hpp>

#ifndef BOOST_MATH_STANDALONE
#include <boost/config.hpp>

#else // Things from boost/config that are required, and easy to replicate

#define BOOST_PREVENT_MACRO_SUBSTITUTION
#define BOOST_MATH_NO_REAL_CONCEPT_TESTS
#define BOOST_MATH_NO_DISTRIBUTION_CONCEPT_TESTS
#define BOOST_MATH_NO_LEXICAL_CAST
#define TEST_STD

#if (__cplusplus > 201400L || _MSVC_LANG > 201400L)
#define BOOST_CXX14_CONSTEXPR constexpr
#else
#define BOOST_CXX14_CONSTEXPR
#define BOOST_NO_CXX14_CONSTEXPR
#endif // BOOST_CXX14_CONSTEXPR

#if (__cplusplus > 201700L || _MSVC_LANG > 201700L)
#define BOOST_IF_CONSTEXPR if constexpr
#if !__has_include(<execution>)
#define BOOST_NO_CXX17_HDR_EXECUTION
#endif
#else
#define BOOST_IF_CONSTEXPR if
#define BOOST_NO_CXX17_IF_CONSTEXPR
#define BOOST_NO_CXX17_HDR_EXECUTION
#endif

#define BOOST_JOIN(X, Y) BOOST_DO_JOIN(X, Y)
#define BOOST_DO_JOIN(X, Y) BOOST_DO_JOIN2(X,Y)
#define BOOST_DO_JOIN2(X, Y) X##Y

#define BOOST_STRINGIZE(X) BOOST_DO_STRINGIZE(X)
#define BOOST_DO_STRINGIZE(X) #X

#ifdef BOOST_DISABLE_THREADS // No threads, do nothing
// Detect thread support via STL implementation
#elif defined(__has_include)
#  if !__has_include(<thread>) || !__has_include(<mutex>) || !__has_include(<future>) || !__has_include(<atomic>)
#     define BOOST_DISABLE_THREADS
#  else
#     define BOOST_HAS_THREADS
#  endif 
#else
#  define BOOST_HAS_THREADS // The default assumption is that the machine has threads
#endif // Thread Support

#ifdef BOOST_DISABLE_THREADS
#  define BOOST_NO_CXX11_HDR_ATOMIC
#  define BOOST_NO_CXX11_HDR_FUTURE
#  define BOOST_NO_CXX11_HDR_THREAD
#  define BOOST_NO_CXX11_THREAD_LOCAL
#endif // BOOST_DISABLE_THREADS

#endif // BOOST_MATH_STANDALONE

#include <algorithm>  // for min and max
#include <limits>
#include <cmath>
#include <climits>
#include <cfloat>
#if (defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__))
#  include <math.h>
#endif

#include <boost/math/tools/user.hpp>

#if (defined(__CYGWIN__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__EMSCRIPTEN__)\
   || (defined(__hppa) && !defined(__OpenBSD__)) || (defined(__NO_LONG_DOUBLE_MATH) && (DBL_MANT_DIG != LDBL_MANT_DIG))) \
   && !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
#  define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
#endif

#ifdef __IBMCPP__
//
// For reasons I don't understand, the tests with IMB's compiler all
// pass at long double precision, but fail with real_concept, those tests
// are disabled for now.  (JM 2012).
#ifndef BOOST_MATH_NO_REAL_CONCEPT_TESTS
#  define BOOST_MATH_NO_REAL_CONCEPT_TESTS
#endif // BOOST_MATH_NO_REAL_CONCEPT_TESTS
#endif
#ifdef sun
// Any use of __float128 in program startup code causes a segfault  (tested JM 2015, Solaris 11).
#  define BOOST_MATH_DISABLE_FLOAT128
#endif
#ifdef __HAIKU__
//
// Not sure what's up with the math detection on Haiku, but linking fails with
// float128 code enabled, and we don't have an implementation of __expl, so
// disabling long double functions for now as well.
#  define BOOST_MATH_DISABLE_FLOAT128
#  define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
#endif
#if (defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)) && ((LDBL_MANT_DIG == 106) || (__LDBL_MANT_DIG__ == 106)) && !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
//
// Darwin's rather strange "double double" is rather hard to
// support, it should be possible given enough effort though...
//
#  define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
#endif
#if !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS) && (LDBL_MANT_DIG == 106) && (LDBL_MIN_EXP > DBL_MIN_EXP)
//
// Generic catch all case for gcc's "double-double" long double type.
// We do not support this as it's not even remotely IEEE conforming:
//
#  define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
#endif
#if defined(unix) && defined(__INTEL_COMPILER) && (__INTEL_COMPILER <= 1000) && !defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
//
// Intel compiler prior to version 10 has sporadic problems
// calling the long double overloads of the std lib math functions:
// calling ::powl is OK, but std::pow(long double, long double) 
// may segfault depending upon the value of the arguments passed 
// and the specific Linux distribution.
//
// We'll be conservative and disable long double support for this compiler.
//
// Comment out this #define and try building the tests to determine whether
// your Intel compiler version has this issue or not.
//
#  define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
#endif
#if defined(unix) && defined(__INTEL_COMPILER)
//
// Intel compiler has sporadic issues compiling std::fpclassify depending on
// the exact OS version used.  Use our own code for this as we know it works
// well on Intel processors:
//
#define BOOST_MATH_DISABLE_STD_FPCLASSIFY
#endif

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
   // Better safe than sorry, our tests don't support hardware exceptions:
#  define BOOST_MATH_CONTROL_FP _control87(MCW_EM,MCW_EM)
#endif

#ifdef __IBMCPP__
#  define BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS
#endif

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901))
#  define BOOST_MATH_USE_C99
#endif

#if (defined(__hpux) && !defined(__hppa))
#  define BOOST_MATH_USE_C99
#endif

#if defined(__GNUC__) && defined(_GLIBCXX_USE_C99)
#  define BOOST_MATH_USE_C99
#endif

#if defined(_LIBCPP_VERSION) && !defined(_MSC_VER)
#  define BOOST_MATH_USE_C99
#endif

#if defined(__CYGWIN__) || defined(__HP_aCC) || defined(__INTEL_COMPILER) \
  || defined(BOOST_NO_NATIVE_LONG_DOUBLE_FP_CLASSIFY) \
  || (defined(__GNUC__) && !defined(BOOST_MATH_USE_C99))\
  || defined(BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS)
#  define BOOST_MATH_NO_NATIVE_LONG_DOUBLE_FP_CLASSIFY
#endif

#if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x590)

namespace boost { namespace math { namespace tools { namespace detail {
template <typename T>
struct type {};

template <typename T, T n>
struct non_type {};
}}}} // Namespace boost, math tools, detail

#  define BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(t)              boost::math::tools::detail::type<t>* = 0
#  define BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(t)         boost::math::tools::detail::type<t>*
#  define BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE(t, v)       boost::math::tools::detail::non_type<t, v>* = 0
#  define BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE_SPEC(t, v)  boost::math::tools::detail::non_type<t, v>*

#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(t)         \
             , BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(t)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE_SPEC(t)    \
             , BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(t)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_NON_TYPE(t, v)  \
             , BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE(t, v)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_NON_TYPE_SPEC(t, v)  \
             , BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE_SPEC(t, v)

#else

// no workaround needed: expand to nothing

#  define BOOST_MATH_EXPLICIT_TEMPLATE_TYPE(t)
#  define BOOST_MATH_EXPLICIT_TEMPLATE_TYPE_SPEC(t)
#  define BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE(t, v)
#  define BOOST_MATH_EXPLICIT_TEMPLATE_NON_TYPE_SPEC(t, v)

#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE(t)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_TYPE_SPEC(t)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_NON_TYPE(t, v)
#  define BOOST_MATH_APPEND_EXPLICIT_TEMPLATE_NON_TYPE_SPEC(t, v)


#endif // __SUNPRO_CC

#if (defined(__SUNPRO_CC) || defined(__hppa) || defined(__GNUC__)) && !defined(BOOST_MATH_SMALL_CONSTANT)
// Sun's compiler emits a hard error if a constant underflows,
// as does aCC on PA-RISC, while gcc issues a large number of warnings:
#  define BOOST_MATH_SMALL_CONSTANT(x) 0.0
#else
#  define BOOST_MATH_SMALL_CONSTANT(x) x
#endif

//
// Tune performance options for specific compilers:
//
#ifdef _MSC_VER
#  define BOOST_MATH_POLY_METHOD 2
#if _MSC_VER <= 1900
#  define BOOST_MATH_RATIONAL_METHOD 1
#else
#  define BOOST_MATH_RATIONAL_METHOD 2
#endif
#if _MSC_VER > 1900
#  define BOOST_MATH_INT_TABLE_TYPE(RT, IT) RT
#  define BOOST_MATH_INT_VALUE_SUFFIX(RV, SUF) RV##.0L
#endif

#elif defined(__INTEL_COMPILER)
#  define BOOST_MATH_POLY_METHOD 2
#  define BOOST_MATH_RATIONAL_METHOD 1

#elif defined(__GNUC__)
#if __GNUC__ < 4
#  define BOOST_MATH_POLY_METHOD 3
#  define BOOST_MATH_RATIONAL_METHOD 3
#  define BOOST_MATH_INT_TABLE_TYPE(RT, IT) RT
#  define BOOST_MATH_INT_VALUE_SUFFIX(RV, SUF) RV##.0L
#else
#  define BOOST_MATH_POLY_METHOD 3
#  define BOOST_MATH_RATIONAL_METHOD 3
#endif

#elif defined(__clang__)

#if __clang__ > 6
#  define BOOST_MATH_POLY_METHOD 3
#  define BOOST_MATH_RATIONAL_METHOD 3
#  define BOOST_MATH_INT_TABLE_TYPE(RT, IT) RT
#  define BOOST_MATH_INT_VALUE_SUFFIX(RV, SUF) RV##.0L
#endif

#endif

//
// noexcept support:
//
#include <type_traits>
#define BOOST_MATH_NOEXCEPT(T) noexcept(std::is_floating_point<T>::value)
#define BOOST_MATH_IS_FLOAT(T) (std::is_floating_point<T>::value)

//
// The maximum order of polynomial that will be evaluated 
// via an unrolled specialisation:
//
#ifndef BOOST_MATH_MAX_POLY_ORDER
#  define BOOST_MATH_MAX_POLY_ORDER 20
#endif 
//
// Set the method used to evaluate polynomials and rationals:
//
#ifndef BOOST_MATH_POLY_METHOD
#  define BOOST_MATH_POLY_METHOD 2
#endif 
#ifndef BOOST_MATH_RATIONAL_METHOD
#  define BOOST_MATH_RATIONAL_METHOD 1
#endif 
//
// decide whether to store constants as integers or reals:
//
#ifndef BOOST_MATH_INT_TABLE_TYPE
#  define BOOST_MATH_INT_TABLE_TYPE(RT, IT) IT
#endif
#ifndef BOOST_MATH_INT_VALUE_SUFFIX
#  define BOOST_MATH_INT_VALUE_SUFFIX(RV, SUF) RV##SUF
#endif
//
// And then the actual configuration:
//
#if defined(BOOST_MATH_STANDALONE) && defined(_GLIBCXX_USE_FLOAT128) && defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__) && !defined(__STRICT_ANSI__) \
   && !defined(BOOST_MATH_DISABLE_FLOAT128) && !defined(BOOST_MATH_USE_FLOAT128)
#  define BOOST_MATH_USE_FLOAT128
#elif defined(BOOST_HAS_FLOAT128) && !defined(BOOST_MATH_USE_FLOAT128)
#  define BOOST_MATH_USE_FLOAT128
#endif
#ifdef BOOST_MATH_USE_FLOAT128
//
// Only enable this when the compiler really is GCC as clang and probably 
// intel too don't support __float128 yet :-(
//
#  if defined(__INTEL_COMPILER) && defined(__GNUC__)
#    if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 6))
#      define BOOST_MATH_FLOAT128_TYPE __float128
#    endif
#  elif defined(__GNUC__)
#      define BOOST_MATH_FLOAT128_TYPE __float128
#  endif

#  ifndef BOOST_MATH_FLOAT128_TYPE
#      define BOOST_MATH_FLOAT128_TYPE _Quad
#  endif
#endif
//
// Check for WinCE with no iostream support:
//
#if defined(_WIN32_WCE) && !defined(__SGI_STL_PORT)
#  define BOOST_MATH_NO_LEXICAL_CAST
#endif

//
// Helper macro for controlling the FP behaviour:
//
#ifndef BOOST_MATH_CONTROL_FP
#  define BOOST_MATH_CONTROL_FP
#endif
//
// Helper macro for using statements:
//
#define BOOST_MATH_STD_USING_CORE \
   using std::abs;\
   using std::acos;\
   using std::cos;\
   using std::fmod;\
   using std::modf;\
   using std::tan;\
   using std::asin;\
   using std::cosh;\
   using std::frexp;\
   using std::pow;\
   using std::tanh;\
   using std::atan;\
   using std::exp;\
   using std::ldexp;\
   using std::sin;\
   using std::atan2;\
   using std::fabs;\
   using std::log;\
   using std::sinh;\
   using std::ceil;\
   using std::floor;\
   using std::log10;\
   using std::sqrt;

#define BOOST_MATH_STD_USING BOOST_MATH_STD_USING_CORE

namespace boost{ namespace math{
namespace tools
{

template <class T>
inline T max BOOST_PREVENT_MACRO_SUBSTITUTION(T a, T b, T c) BOOST_MATH_NOEXCEPT(T)
{
   return (std::max)((std::max)(a, b), c);
}

template <class T>
inline T max BOOST_PREVENT_MACRO_SUBSTITUTION(T a, T b, T c, T d) BOOST_MATH_NOEXCEPT(T)
{
   return (std::max)((std::max)(a, b), (std::max)(c, d));
}

} // namespace tools

template <class T>
void suppress_unused_variable_warning(const T&) BOOST_MATH_NOEXCEPT(T)
{
}

namespace detail{

template <class T>
struct is_integer_for_rounding
{
   static constexpr bool value = std::is_integral<T>::value || (std::numeric_limits<T>::is_specialized && std::numeric_limits<T>::is_integer);
};

}

}} // namespace boost namespace math

#ifdef __GLIBC_PREREQ
#  if __GLIBC_PREREQ(2,14)
#     define BOOST_MATH_HAVE_FIXED_GLIBC
#  endif
#endif

#if ((defined(__linux__) && !defined(__UCLIBC__) && !defined(BOOST_MATH_HAVE_FIXED_GLIBC)) || defined(__QNX__) || defined(__IBMCPP__))
//
// This code was introduced in response to this glibc bug: http://sourceware.org/bugzilla/show_bug.cgi?id=2445
// Basically powl and expl can return garbage when the result is small and certain exception flags are set
// on entrance to these functions.  This appears to have been fixed in Glibc 2.14 (May 2011).
// Much more information in this message thread: https://groups.google.com/forum/#!topic/boost-list/ZT99wtIFlb4
//

#include <cfenv>

#  ifdef FE_ALL_EXCEPT

namespace boost{ namespace math{
   namespace detail
   {
   struct fpu_guard
   {
      fpu_guard()
      {
         fegetexceptflag(&m_flags, FE_ALL_EXCEPT);
         feclearexcept(FE_ALL_EXCEPT);
      }
      ~fpu_guard()
      {
         fesetexceptflag(&m_flags, FE_ALL_EXCEPT);
      }
   private:
      fexcept_t m_flags;
   };

   } // namespace detail
   }} // namespaces

#    define BOOST_FPU_EXCEPTION_GUARD boost::math::detail::fpu_guard local_guard_object;
#    define BOOST_MATH_INSTRUMENT_FPU do{ fexcept_t cpu_flags; fegetexceptflag(&cpu_flags, FE_ALL_EXCEPT); BOOST_MATH_INSTRUMENT_VARIABLE(cpu_flags); } while(0); 

#  else

#    define BOOST_FPU_EXCEPTION_GUARD
#    define BOOST_MATH_INSTRUMENT_FPU

#  endif

#else // All other platforms.
#  define BOOST_FPU_EXCEPTION_GUARD
#  define BOOST_MATH_INSTRUMENT_FPU
#endif

#ifdef BOOST_MATH_INSTRUMENT

#  include <iostream>
#  include <iomanip>
#  include <typeinfo>

#  define BOOST_MATH_INSTRUMENT_CODE(x) \
      std::cout << std::setprecision(35) << __FILE__ << ":" << __LINE__ << " " << x << std::endl;
#  define BOOST_MATH_INSTRUMENT_VARIABLE(name) BOOST_MATH_INSTRUMENT_CODE(#name << " = " << name)

#else

#  define BOOST_MATH_INSTRUMENT_CODE(x)
#  define BOOST_MATH_INSTRUMENT_VARIABLE(name)

#endif

//
// Thread local storage:
//
#ifndef BOOST_DISABLE_THREADS
#  define BOOST_MATH_THREAD_LOCAL thread_local
#else
#  define BOOST_MATH_THREAD_LOCAL 
#endif

//
// Some mingw flavours have issues with thread_local and types with non-trivial destructors
// See https://sourceforge.net/p/mingw-w64/bugs/527/
//
#if (defined(__MINGW32__) && (__GNUC__ < 9) && !defined(__clang__))
#  define BOOST_MATH_NO_THREAD_LOCAL_WITH_NON_TRIVIAL_TYPES
#endif


//
// Can we have constexpr tables?
//
#if (!defined(BOOST_NO_CXX14_CONSTEXPR)) || (defined(_MSC_VER) && _MSC_VER >= 1910)
#define BOOST_MATH_HAVE_CONSTEXPR_TABLES
#define BOOST_MATH_CONSTEXPR_TABLE_FUNCTION constexpr
#else
#define BOOST_MATH_CONSTEXPR_TABLE_FUNCTION
#endif


#endif // BOOST_MATH_TOOLS_CONFIG_HPP




