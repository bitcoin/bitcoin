/*
Copyright Rene Rivera 2012-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_BSD_NET_H
#define BOOST_PREDEF_OS_BSD_NET_H

#include <boost/predef/os/bsd.h>

/* tag::reference[]
= `BOOST_OS_BSD_NET`

http://en.wikipedia.org/wiki/Netbsd[NetBSD] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__NETBSD__+` | {predef_detection}
| `+__NetBSD__+` | {predef_detection}

| `+__NETBSD_version+` | V.R.P
| `NetBSD0_8` | 0.8.0
| `NetBSD0_9` | 0.9.0
| `NetBSD1_0` | 1.0.0
| `+__NetBSD_Version+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(__NETBSD__) || defined(__NetBSD__) \
    )
#   ifndef BOOST_OS_BSD_AVAILABLE
#       undef BOOST_OS_BSD
#       define BOOST_OS_BSD BOOST_VERSION_NUMBER_AVAILABLE
#       define BOOST_OS_BSD_AVAILABLE
#   endif
#   undef BOOST_OS_BSD_NET
#   if defined(__NETBSD__)
#       if defined(__NETBSD_version)
#           if __NETBSD_version < 500000
#               define BOOST_OS_BSD_NET \
                    BOOST_PREDEF_MAKE_10_VRP000(__NETBSD_version)
#           else
#               define BOOST_OS_BSD_NET \
                    BOOST_PREDEF_MAKE_10_VRR000(__NETBSD_version)
#           endif
#       else
#           define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   elif defined(__NetBSD__)
#       if !defined(BOOST_OS_BSD_NET) && defined(NetBSD0_8)
#           define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER(0,8,0)
#       endif
#       if !defined(BOOST_OS_BSD_NET) && defined(NetBSD0_9)
#           define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER(0,9,0)
#       endif
#       if !defined(BOOST_OS_BSD_NET) && defined(NetBSD1_0)
#           define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER(1,0,0)
#       endif
#       if !defined(BOOST_OS_BSD_NET) && defined(__NetBSD_Version)
#           define BOOST_OS_BSD_NET \
                BOOST_PREDEF_MAKE_10_VVRR00PP00(__NetBSD_Version)
#       endif
#       if !defined(BOOST_OS_BSD_NET)
#           define BOOST_OS_BSD_NET BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   endif
#endif

#if BOOST_OS_BSD_NET
#   define BOOST_OS_BSD_NET_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_BSD_NET_NAME "NetBSD"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_BSD_NET,BOOST_OS_BSD_NET_NAME)
