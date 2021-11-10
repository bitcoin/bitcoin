/* Copyright 2006-2018 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_DETAIL_NOT_PLACEHOLDER_EXPR_HPP
#define BOOST_FLYWEIGHT_DETAIL_NOT_PLACEHOLDER_EXPR_HPP

#if defined(_MSC_VER)
#pragma once
#endif

/* BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION can be inserted at the end
 * of a class template parameter declaration:
 *   template<
 *     typename X0,...,typename Xn
 *     BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION  
 *   >
 *   struct foo...
 * to prevent instantiations from being treated as MPL placeholder
 * expressions in the presence of placeholder arguments; this is useful
 * to avoid masking of a metafunction class nested ::apply during
 * MPL invocation.
 */

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/workaround.hpp>

#if BOOST_WORKAROUND(__GNUC__, <4)||\
    BOOST_WORKAROUND(__GNUC__,==4)&&(__GNUC_MINOR__<2)||\
    BOOST_WORKAROUND(__GNUC__, ==7)&&( __cplusplus>=201703L)||\
    BOOST_WORKAROUND(__GNUC__, >=8)&&( __cplusplus>=201103L)
/* The default trick on which the macro is based, namely adding a int=0
 * defaulted template parameter, does not work in GCC prior to 4.2 due to
 * an unfortunate compiler non-standard extension, as explained in
 *   http://lists.boost.org/boost-users/2007/07/29866.php
 * As it happens, GCC 7 in C++17 mode and GCC 8 (and presumably later) in
 * C++11 mode (and presumably later) go back to this old behavior, anticipating
 * the resolution of CWG DR 150 (see P0522R0).
 * In these cases we resort to an uglier technique, adding defaulted template
 * parameters so as to exceed BOOST_MPL_LIMIT_METAFUNCTION_ARITY.
 */

#include <boost/mpl/limits/arity.hpp>
#include <boost/preprocessor/facilities/intercept.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

#define BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION                  \
BOOST_PP_ENUM_TRAILING_PARAMS(                                        \
  BOOST_MPL_LIMIT_METAFUNCTION_ARITY,typename=int BOOST_PP_INTERCEPT)
#define BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION_DEF              \
BOOST_PP_ENUM_TRAILING_PARAMS(                                        \
  BOOST_MPL_LIMIT_METAFUNCTION_ARITY,typename BOOST_PP_INTERCEPT)

#else
#define BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION  ,int=0
#define BOOST_FLYWEIGHT_NOT_A_PLACEHOLDER_EXPRESSION_DEF  ,int
#endif

#endif
