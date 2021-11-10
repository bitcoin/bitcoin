/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_IA64_H
#define BOOST_PREDEF_ARCHITECTURE_IA64_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_IA64`

http://en.wikipedia.org/wiki/Ia64[Intel Itanium 64] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__ia64__+` | {predef_detection}
| `+_IA64+` | {predef_detection}
| `+__IA64__+` | {predef_detection}
| `+__ia64+` | {predef_detection}
| `+_M_IA64+` | {predef_detection}
| `+__itanium__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_ARCH_IA64 BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__ia64__) || defined(_IA64) || \
    defined(__IA64__) || defined(__ia64) || \
    defined(_M_IA64) || defined(__itanium__)
#   undef BOOST_ARCH_IA64
#   define BOOST_ARCH_IA64 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_IA64
#   define BOOST_ARCH_IA64_AVAILABLE
#endif

#if BOOST_ARCH_IA64
#   undef BOOST_ARCH_WORD_BITS_64
#   define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_IA64_NAME "Intel Itanium 64"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_IA64,BOOST_ARCH_IA64_NAME)
