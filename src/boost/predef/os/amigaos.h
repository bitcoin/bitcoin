/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_AMIGAOS_H
#define BOOST_PREDEF_OS_AMIGAOS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_AMIGAOS`

http://en.wikipedia.org/wiki/AmigaOS[AmigaOS] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `AMIGA` | {predef_detection}
| `+__amigaos__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_AMIGAOS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(AMIGA) || defined(__amigaos__) \
    )
#   undef BOOST_OS_AMIGAOS
#   define BOOST_OS_AMIGAOS BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_OS_AMIGAOS
#   define BOOST_OS_AMIGAOS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_AMIGAOS_NAME "AmigaOS"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_AMIGAOS,BOOST_OS_AMIGAOS_NAME)
