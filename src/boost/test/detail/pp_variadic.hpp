//  (C) Copyright Gennadiy Rozental 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//!@file
//!@brief few helpers for working with variadic macros
// ***************************************************************************

#ifndef BOOST_TEST_PP_VARIADIC_HPP_021515GER
#define BOOST_TEST_PP_VARIADIC_HPP_021515GER

// Boost
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/variadic/size.hpp>

//____________________________________________________________________________//

#if BOOST_PP_VARIADICS

#if BOOST_PP_VARIADICS_MSVC
#  define BOOST_TEST_INVOKE_VARIADIC( tool, ... ) BOOST_PP_CAT( tool (__VA_ARGS__), )
#else
#  define BOOST_TEST_INVOKE_VARIADIC( tool, ... ) tool (__VA_ARGS__)
#endif

//____________________________________________________________________________//

/// if sizeof(__VA_ARGS__) == N: F1(__VA_ARGS__)
/// else:                        F2(__VA_ARGS__)
#define BOOST_TEST_INVOKE_IF_N_ARGS( N, F1, F2, ... )               \
    BOOST_TEST_INVOKE_VARIADIC(                                     \
        BOOST_PP_IIF(                                               \
            BOOST_PP_EQUAL(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), N), \
            F1,                                                     \
            F2),                                                    \
        __VA_ARGS__ )                                               \
/**/

//____________________________________________________________________________//

#endif  /* BOOST_PP_VARIADICS */

#endif // BOOST_TEST_PP_VARIADIC_HPP_021515GER

// EOF
