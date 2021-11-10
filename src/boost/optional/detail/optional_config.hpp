// Copyright (C) 2003, 2008 Fernando Luis Cacciola Carballal.
// Copyright (C) 2015 - 2017 Andrzej Krzemienski.
//
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/optional for documentation.
//
// You are welcome to contact the author at:
//  akrzemi1@gmail.com

#ifndef BOOST_OPTIONAL_DETAIL_OPTIONAL_CONFIG_AJK_28JAN2015_HPP
#define BOOST_OPTIONAL_DETAIL_OPTIONAL_CONFIG_AJK_28JAN2015_HPP

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#if (defined BOOST_NO_CXX11_RVALUE_REFERENCES) || (defined BOOST_OPTIONAL_CONFIG_NO_RVALUE_REFERENCES)
# define BOOST_OPTIONAL_DETAIL_NO_RVALUE_REFERENCES
#endif

#if BOOST_WORKAROUND(BOOST_INTEL_CXX_VERSION,<=700)
// AFAICT only Intel 7 correctly resolves the overload set
// that includes the in-place factory taking functions,
// so for the other icc versions, in-place factory support
// is disabled
# define BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT
#endif

#if BOOST_WORKAROUND(BOOST_BORLANDC, <= 0x551)
// BCB (5.5.1) cannot parse the nested template struct in an inplace factory.
# define BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT
#endif

#if !defined(BOOST_OPTIONAL_NO_INPLACE_FACTORY_SUPPORT) \
    && defined BOOST_BCB_PARTIAL_SPECIALIZATION_BUG
// BCB (up to 5.64) has the following bug:
//   If there is a member function/operator template of the form
//     template<class Expr> mfunc( Expr expr ) ;
//   some calls are resolved to this even if there are other better matches.
//   The effect of this bug is that calls to converting ctors and assignments
//   are incorrectly sink to this general catch-all member function template as shown above.
# define BOOST_OPTIONAL_WEAK_OVERLOAD_RESOLUTION
#endif

#if !defined(BOOST_NO_MAY_ALIAS)
// GCC since 3.3 and some other compilers have may_alias attribute that helps to alleviate
// optimizer issues with regard to violation of the strict aliasing rules. The optional< T >
// storage type is marked with this attribute in order to let the compiler know that it will
// alias objects of type T and silence compilation warnings.
# define BOOST_OPTIONAL_DETAIL_USE_ATTRIBUTE_MAY_ALIAS
#endif

#if (defined(_MSC_VER) && _MSC_VER <= 1800)
// on MSCV 2013 and earlier an unwanted temporary is created when you assign from
// a const lvalue of integral type. Thus we bind not to the original address but
// to a temporary. 
# define BOOST_OPTIONAL_CONFIG_NO_PROPER_ASSIGN_FROM_CONST_INT
#endif

#if (defined __GNUC__) && (!defined BOOST_INTEL_CXX_VERSION) && (!defined __clang__)
// On some GCC versions an unwanted temporary is created when you copy-initialize
// from a const lvalue of integral type. Thus we bind not to the original address but
// to a temporary.

# if (__GNUC__ < 4)
#  define BOOST_OPTIONAL_CONFIG_NO_PROPER_CONVERT_FROM_CONST_INT
# endif

# if (__GNUC__ == 4 && __GNUC_MINOR__ <= 5)
#  define BOOST_OPTIONAL_CONFIG_NO_PROPER_CONVERT_FROM_CONST_INT
# endif

# if (__GNUC__ == 5 && __GNUC_MINOR__ < 2)
#  define BOOST_OPTIONAL_CONFIG_NO_PROPER_CONVERT_FROM_CONST_INT
# endif

# if (__GNUC__ == 5 && __GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ == 0)
#  define BOOST_OPTIONAL_CONFIG_NO_PROPER_CONVERT_FROM_CONST_INT
# endif

#endif // defined(__GNUC__)

#if (defined __GNUC__) && (!defined BOOST_NO_CXX11_RVALUE_REFERENCES)
// On some initial rvalue reference implementations GCC does it in a strange way,
// preferring perfect-forwarding constructor to implicit copy constructor.

# if (__GNUC__ == 4 && __GNUC_MINOR__ == 4)
#  define BOOST_OPTIONAL_CONFIG_NO_LEGAL_CONVERT_FROM_REF
# endif

# if (__GNUC__ == 4 && __GNUC_MINOR__ == 5)
#  define BOOST_OPTIONAL_CONFIG_NO_LEGAL_CONVERT_FROM_REF
# endif

#endif // defined(__GNUC__)

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && !defined(BOOST_NO_CXX11_DECLTYPE) && !BOOST_WORKAROUND(BOOST_MSVC, < 1800) && !BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40500) && !defined(__SUNPRO_CC)
  // this condition is a copy paste from is_constructible.hpp
  // I also disable SUNPRO, as it seems not to support type_traits correctly
#else
# define BOOST_OPTIONAL_DETAIL_NO_IS_CONSTRUCTIBLE_TRAIT
#endif

#if defined __SUNPRO_CC
# define BOOST_OPTIONAL_DETAIL_NO_SFINAE_FRIENDLY_CONSTRUCTORS
#elif (defined _MSC_FULL_VER) && (_MSC_FULL_VER < 190023026)
# define BOOST_OPTIONAL_DETAIL_NO_SFINAE_FRIENDLY_CONSTRUCTORS
#elif defined BOOST_GCC && !defined BOOST_GCC_CXX11
# define BOOST_OPTIONAL_DETAIL_NO_SFINAE_FRIENDLY_CONSTRUCTORS
#elif defined BOOST_GCC_VERSION && BOOST_GCC_VERSION < 40800
# define BOOST_OPTIONAL_DETAIL_NO_SFINAE_FRIENDLY_CONSTRUCTORS
#endif


// Detect suport for defaulting move operations
// (some older compilers implement rvalue references,
// defaulted funcitons but move operations are not special members and cannot be defaulted)

#ifdef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
# define BOOST_OPTIONAL_DETAIL_NO_DEFAULTED_MOVE_FUNCTIONS
#elif BOOST_WORKAROUND(BOOST_MSVC, < 1900)
# define BOOST_OPTIONAL_DETAIL_NO_DEFAULTED_MOVE_FUNCTIONS
#elif BOOST_WORKAROUND(BOOST_GCC_VERSION, < 40600)
# define BOOST_OPTIONAL_DETAIL_NO_DEFAULTED_MOVE_FUNCTIONS
#endif


#ifdef BOOST_OPTIONAL_CONFIG_NO_DIRECT_STORAGE_SPEC
# define BOOST_OPTIONAL_DETAIL_NO_DIRECT_STORAGE_SPEC
#endif


#endif // header guard
