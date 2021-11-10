/*
Copyright Rene Rivera 2012-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_BSD_DRAGONFLY_H
#define BOOST_PREDEF_OS_BSD_DRAGONFLY_H

#include <boost/predef/os/bsd.h>

/* tag::reference[]
= `BOOST_OS_BSD_DRAGONFLY`

http://en.wikipedia.org/wiki/DragonFly_BSD[DragonFly BSD] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__DragonFly__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_OS_BSD_DRAGONFLY BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__DragonFly__) \
    )
#   ifndef BOOST_OS_BSD_AVAILABLE
#       undef BOOST_OS_BSD
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER_AVAILABLE
#       define BOOST_OS_BSD_AVAILABLE
#   endif
#   undef BOOST_OS_BSD_DRAGONFLY
#   if defined(__DragonFly__)
#       define BOOST_OS_DRAGONFLY_BSD BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_OS_BSD_DRAGONFLY
#   define BOOST_OS_BSD_DRAGONFLY_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_BSD_DRAGONFLY_NAME "DragonFly BSD"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BSD_DRAGONFLY,BOOST_OS_BSD_DRAGONFLY_NAME)
