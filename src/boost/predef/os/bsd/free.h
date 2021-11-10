/*
Copyright Rene Rivera 2012-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_BSD_FREE_H
#define BOOST_PREDEF_OS_BSD_FREE_H

#include <boost/predef/os/bsd.h>

/* tag::reference[]
= `BOOST_OS_BSD_FREE`

http://en.wikipedia.org/wiki/Freebsd[FreeBSD] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__FreeBSD__+` | {predef_detection}

| `+__FreeBSD_version+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_OS_BSD_FREE BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__FreeBSD__) \
    )
#   ifndef BOOST_OS_BSD_AVAILABLE
#       undef BOOST_OS_BSD
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER_AVAILABLE
#       define BOOST_OS_BSD_AVAILABLE
#   endif
#   undef BOOST_OS_BSD_FREE
#   include <sys/param.h>
#   if defined(__FreeBSD_version)
#       if __FreeBSD_version == 491000
#           define BOOST_OS_BSD_FREE \
                BOOST_VERSION_NUMBER(4, 10, 0)
#       elif __FreeBSD_version == 492000
#           define BOOST_OS_BSD_FREE \
                BOOST_VERSION_NUMBER(4, 11, 0)
#       elif __FreeBSD_version < 500000
#           define BOOST_OS_BSD_FREE \
                BOOST_PREDEF_MAKE_10_VRPPPP(__FreeBSD_version)
#       else
#           define BOOST_OS_BSD_FREE \
                BOOST_PREDEF_MAKE_10_VVRRPPP(__FreeBSD_version)
#       endif
#   else
#       define BOOST_OS_BSD_FREE BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_OS_BSD_FREE
#   define BOOST_OS_BSD_FREE_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_BSD_FREE_NAME "Free BSD"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BSD_FREE,BOOST_OS_BSD_FREE_NAME)
