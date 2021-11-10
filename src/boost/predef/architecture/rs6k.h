/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_RS6K_H
#define BOOST_PREDEF_ARCHITECTURE_RS6K_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_RS6000`

http://en.wikipedia.org/wiki/RS/6000[RS/6000] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__THW_RS6000+` | {predef_detection}
| `+_IBMR2+` | {predef_detection}
| `+_POWER+` | {predef_detection}
| `+_ARCH_PWR+` | {predef_detection}
| `+_ARCH_PWR2+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_ARCH_RS6000 BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__THW_RS6000) || defined(_IBMR2) || \
    defined(_POWER) || defined(_ARCH_PWR) || \
    defined(_ARCH_PWR2)
#   undef BOOST_ARCH_RS6000
#   define BOOST_ARCH_RS6000 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_RS6000
#   define BOOST_ARCH_RS6000_AVAILABLE
#endif

#if BOOST_ARCH_RS6000
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_RS6000_NAME "RS/6000"

#define BOOST_ARCH_PWR BOOST_ARCH_RS6000

#if BOOST_ARCH_PWR
#   define BOOST_ARCH_PWR_AVAILABLE
#endif

#if BOOST_ARCH_PWR
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_PWR_NAME BOOST_ARCH_RS6000_NAME

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_RS6000,BOOST_ARCH_RS6000_NAME)
