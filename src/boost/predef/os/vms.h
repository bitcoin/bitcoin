/*
Copyright Rene Rivera 2011-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OS_VMS_H
#define BOOST_PREDEF_OS_VMS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_OS_VMS`

http://en.wikipedia.org/wiki/Vms[VMS] operating system.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `VMS` | {predef_detection}
| `+__VMS+` | {predef_detection}

| `+__VMS_VER+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_OS_VMS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if !defined(BOOST_PREDEF_DETAIL_OS_DETECTED) && ( \
    defined(VMS) || defined(__VMS) \
    )
#   undef BOOST_OS_VMS
#   if defined(__VMS_VER)
#       define BOOST_OS_VMS BOOST_PREDEF_MAKE_10_VVRR00PP00(__VMS_VER)
#   else
#       define BOOST_OS_VMS BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_OS_VMS
#   define BOOST_OS_VMS_AVAILABLE
#   include <boost/predef/detail/os_detected.h>
#endif

#define BOOST_OS_VMS_NAME "VMS"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_OS_VMS,BOOST_OS_VMS_NAME)
