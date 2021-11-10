/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_SOLARIS_H
#define BOOST_PREDEF_OS_SOLARIS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_SOLARIS`

http://en.wikipedia.org/wiki/Solaris_Operating_Environment[Solaris] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `sun` | {predef_detection}
| `+__sun+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_SOLARIS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(sun) || defined(__sun) \
    )
#   undef BOOST_OS_SOLARIS
#   define BOOST_OS_SOLARIS BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_OS_SOLARIS
#   define BOOST_OS_SOLARIS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_SOLARIS_NAME "Solaris"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_SOLARIS,BOOST_OS_SOLARIS_NAME)
