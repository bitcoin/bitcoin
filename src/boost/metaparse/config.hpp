#ifndef BOOST_METAPARSE_CONFIG_HPP
#define BOOST_METAPARSE_CONFIG_HPP

// Copyright Abel Sinkovics (abel@sinkovics.hu)  2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>

/*
 * Compiler workarounds
 */

// BOOST_NO_CXX11_CONSTEXPR is not defined in gcc 4.6
#if \
  defined BOOST_NO_CXX11_CONSTEXPR || defined BOOST_NO_CONSTEXPR || ( \
    !defined __clang__ && defined __GNUC__ \
    && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)) \
  )

#  define BOOST_NO_CONSTEXPR_C_STR

#endif

/*
 * C++ standard to use
 */

// Allow overriding this
#ifndef BOOST_METAPARSE_STD

// special-case MSCV >= 1900 as its constexpr support is good enough for
// Metaparse
#  if \
    !defined BOOST_NO_CXX11_VARIADIC_TEMPLATES \
    && !defined BOOST_NO_VARIADIC_TEMPLATES \
    && ( \
      (!defined BOOST_NO_CONSTEXPR && !defined BOOST_NO_CXX11_CONSTEXPR) \
      || (defined _MSC_VER && _MSC_VER >= 1900) \
    ) \
    && (!defined BOOST_GCC || BOOST_GCC >= 40700)

#    if !defined BOOST_NO_CXX14_CONSTEXPR

#      define BOOST_METAPARSE_STD 2014

#    else

#      define BOOST_METAPARSE_STD 2011

#    endif

#  else

#    define BOOST_METAPARSE_STD 1998

#  endif

#endif

/*
 * Metaparse config
 */

#if BOOST_METAPARSE_STD >= 2011
#  define BOOST_METAPARSE_VARIADIC_STRING
#endif

#endif

