/*
Copyright Rene Rivera 2013-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_BLACKFIN_H
#define BOOST_PREDEF_ARCHITECTURE_BLACKFIN_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_BLACKFIN`

Blackfin Processors from Analog Devices.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__bfin__+` | {predef_detection}
| `+__BFIN__+` | {predef_detection}
| `bfin` | {predef_detection}
| `BFIN` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_ARCH_BLACKFIN BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__bfin__) || defined(__BFIN__) || \
    defined(bfin) || defined(BFIN)
#   undef BOOST_ARCH_BLACKFIN
#   define BOOST_ARCH_BLACKFIN BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_BLACKFIN
#   define BOOST_ARCH_BLACKFIN_AVAILABLE
#endif

#if BOOST_ARCH_BLACKFIN
#   undef BOOST_ARCH_WORD_BITS_16
#   define BOOST_ARCH_WORD_BITS_16 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_BLACKFIN_NAME "Blackfin"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_BLACKFIN,BOOST_ARCH_BLACKFIN_NAME)
