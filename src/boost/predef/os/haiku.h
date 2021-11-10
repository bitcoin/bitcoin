/*
Copyright Jessica Hamilton 2014
Copyright Rene Rivera 2014-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_HAIKU_H
#define BOOST_PREDEF_OS_HAIKU_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_HAIKU`

http://en.wikipedia.org/wiki/Haiku_(operating_system)[Haiku] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__HAIKU__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_HAIKU BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__HAIKU__) \
    )
#   undef BOOST_OS_HAIKU
#   define BOOST_OS_HAIKU BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_OS_HAIKU
#   define BOOST_OS_HAIKU_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_HAIKU_NAME "Haiku"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_HAIKU,BOOST_OS_HAIKU_NAME)
