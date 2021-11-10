/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_PLAT_MINGW32_H
#define BOOST_PREDEF_PLAT_MINGW32_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_PLAT_MINGW32`

http://www.mingw.org/[MinGW] platform.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__MINGW32__+` | {predef_detection}

| `+__MINGW32_VERSION_MAJOR+`, `+__MINGW32_VERSION_MINOR+` | V.R.0
|===
*/ // end::reference[]

#define BOOST_PLAT_MINGW32 BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__MINGW32__)
#   include <_mingw.h>
#   if !defined(BOOST_PLAT_MINGW32_DETECTION) && (defined(__MINGW32_VERSION_MAJOR) && defined(__MINGW32_VERSION_MINOR))
#       define BOOST_PLAT_MINGW32_DETECTION \
            BOOST_VERSION_NUMBER(__MINGW32_VERSION_MAJOR,__MINGW32_VERSION_MINOR,0)
#   endif
#   if !defined(BOOST_PLAT_MINGW32_DETECTION)
#       define BOOST_PLAT_MINGW32_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_PLAT_MINGW32_DETECTION
#   define BOOST_PLAT_MINGW32_AVAILABLE
#   if defined(BOOST_PREDEF_DETAIL_PLAT_DETECTED)
#       define BOOST_PLAT_MINGW32_EMULATED BOOST_PLAT_MINGW32_DETECTION
#   else
#       undef BOOST_PLAT_MINGW32
#       define BOOST_PLAT_MINGW32 BOOST_PLAT_MINGW32_DETECTION
#   endif
#   include <boost/predef/detail/platform_detected.h>
#endif

#define BOOST_PLAT_MINGW32_NAME "MinGW"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_MINGW32,BOOST_PLAT_MINGW32_NAME)

#ifdef BOOST_PLAT_MINGW32_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_MINGW32_EMULATED,BOOST_PLAT_MINGW32_NAME)
#endif
