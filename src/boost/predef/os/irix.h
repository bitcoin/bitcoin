/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_IRIX_H
#define BOOST_PREDEF_OS_IRIX_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_IRIX`

http://en.wikipedia.org/wiki/Irix[IRIX] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `sgi` | {predef_detection}
| `+__sgi+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_IRIX BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(sgi) || defined(__sgi) \
    )
#   undef BOOST_OS_IRIX
#   define BOOST_OS_IRIX BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_OS_IRIX
#   define BOOST_OS_IRIX_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_IRIX_NAME "IRIX"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_IRIX,BOOST_OS_IRIX_NAME)
