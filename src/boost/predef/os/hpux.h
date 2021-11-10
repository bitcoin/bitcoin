/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_HPUX_H
#define BOOST_PREDEF_OS_HPUX_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_HPUX`

http://en.wikipedia.org/wiki/HP-UX[HP-UX] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `hpux` | {predef_detection}
| `+_hpux+` | {predef_detection}
| `+__hpux+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_HPUX BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(hpux) || defined(_hpux) || defined(__hpux) \
    )
#   undef BOOST_OS_HPUX
#   define BOOST_OS_HPUX BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_OS_HPUX
#   define BOOST_OS_HPUX_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_HPUX_NAME "HP-UX"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_HPUX,BOOST_OS_HPUX_NAME)
