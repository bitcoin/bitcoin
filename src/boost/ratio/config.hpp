//  config.hpp  ---------------------------------------------------------------//

//  Copyright 2012 Vicente J. Botet Escriba

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_RATIO_CONFIG_HPP
#define BOOST_RATIO_CONFIG_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>


#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
#  if ! defined BOOST_NO_CXX11_U16STRING
#    define BOOST_NO_CXX11_U16STRING
#  endif
#  if ! defined BOOST_NO_CXX11_U32STRING
#    define BOOST_NO_CXX11_U32STRING
#  endif
#endif


#if !defined BOOST_RATIO_VERSION
#define BOOST_RATIO_VERSION 1
#else
#if BOOST_RATIO_VERSION!=1  && BOOST_RATIO_VERSION!=2
#error "BOOST_RATIO_VERSION must be 1 or 2"
#endif
#endif

#if BOOST_RATIO_VERSION==1
#if ! defined BOOST_RATIO_DONT_PROVIDE_DEPRECATED_FEATURES_SINCE_V2_0_0
#define BOOST_RATIO_PROVIDES_DEPRECATED_FEATURES_SINCE_V2_0_0
#endif
#endif

#if BOOST_RATIO_VERSION==2
#if ! defined BOOST_RATIO_PROVIDES_DEPRECATED_FEATURES_SINCE_V2_0_0
#define BOOST_RATIO_DONT_PROVIDE_DEPRECATED_FEATURES_SINCE_V2_0_0
#endif
#endif

#ifdef INTMAX_C
#define BOOST_RATIO_INTMAX_C(a) INTMAX_C(a)
#elif __cplusplus >= 201103L
#define BOOST_RATIO_INTMAX_C(a) a##LL
#else
#define BOOST_RATIO_INTMAX_C(a) a##L
#endif

#ifdef UINTMAX_C
#define BOOST_RATIO_UINTMAX_C(a) UINTMAX_C(a)
#elif __cplusplus >= 201103L
#define BOOST_RATIO_UINTMAX_C(a) a##ULL
#else
#define BOOST_RATIO_UINTMAX_C(a) a##UL
#endif

#define BOOST_RATIO_INTMAX_T_MAX (0x7FFFFFFFFFFFFFFELL)


#ifndef BOOST_NO_CXX11_STATIC_ASSERT
#define BOOST_RATIO_STATIC_ASSERT(CND, MSG, TYPES) static_assert(CND,MSG)
#elif defined(BOOST_RATIO_USES_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define BOOST_RATIO_STATIC_ASSERT(CND, MSG, TYPES) BOOST_STATIC_ASSERT(CND)
#elif defined(BOOST_RATIO_USES_MPL_ASSERT)
#include <boost/mpl/assert.hpp>
#include <boost/mpl/bool.hpp>
#define BOOST_RATIO_STATIC_ASSERT(CND, MSG, TYPES)                                 \
    BOOST_MPL_ASSERT_MSG(boost::mpl::bool_< (CND) >::type::value, MSG, TYPES)
#else
//~ #elif defined(BOOST_RATIO_USES_ARRAY_ASSERT)
#define BOOST_RATIO_CONCAT(A,B) A##B
#define BOOST_RATIO_NAME(A,B) BOOST_RATIO_CONCAT(A,B)
#define BOOST_RATIO_STATIC_ASSERT(CND, MSG, TYPES) static char BOOST_RATIO_NAME(__boost_ratio_test_,__LINE__)[(CND)?1:-1]
//~ #define BOOST_RATIO_STATIC_ASSERT(CND, MSG, TYPES)
#endif

#if !defined(BOOST_NO_CXX11_STATIC_ASSERT) || !defined(BOOST_RATIO_USES_MPL_ASSERT)
#define BOOST_RATIO_OVERFLOW_IN_ADD "overflow in ratio add"
#define BOOST_RATIO_OVERFLOW_IN_SUB "overflow in ratio sub"
#define BOOST_RATIO_OVERFLOW_IN_MUL "overflow in ratio mul"
#define BOOST_RATIO_OVERFLOW_IN_DIV "overflow in ratio div"
#define BOOST_RATIO_NUMERATOR_IS_OUT_OF_RANGE "ratio numerator is out of range"
#define BOOST_RATIO_DIVIDE_BY_0 "ratio divide by 0"
#define BOOST_RATIO_DENOMINATOR_IS_OUT_OF_RANGE "ratio denominator is out of range"
#endif


//#define BOOST_RATIO_EXTENSIONS

#endif  // header
