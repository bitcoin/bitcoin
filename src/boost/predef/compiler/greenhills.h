/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_GREENHILLS_H
#define BOOST_PREDEF_COMPILER_GREENHILLS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_GHS`

http://en.wikipedia.org/wiki/Green_Hills_Software[Green Hills C/{CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__ghs+` | {predef_detection}
| `+__ghs__+` | {predef_detection}

| `+__GHS_VERSION_NUMBER__+` | V.R.P
| `+__ghs+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_GHS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__ghs) || defined(__ghs__)
#   if !defined(BOOST_COMP_GHS_DETECTION) && defined(__GHS_VERSION_NUMBER__)
#       define BOOST_COMP_GHS_DETECTION BOOST_PREDEF_MAKE_10_VRP(__GHS_VERSION_NUMBER__)
#   endif
#   if !defined(BOOST_COMP_GHS_DETECTION) && defined(__ghs)
#       define BOOST_COMP_GHS_DETECTION BOOST_PREDEF_MAKE_10_VRP(__ghs)
#   endif
#   if !defined(BOOST_COMP_GHS_DETECTION)
#       define BOOST_COMP_GHS_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_GHS_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_GHS_EMULATED BOOST_COMP_GHS_DETECTION
#   else
#       undef BOOST_COMP_GHS
#       define BOOST_COMP_GHS BOOST_COMP_GHS_DETECTION
#   endif
#   define BOOST_COMP_GHS_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_GHS_NAME "Green Hills C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GHS,BOOST_COMP_GHS_NAME)

#ifdef BOOST_COMP_GHS_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GHS_EMULATED,BOOST_COMP_GHS_NAME)
#endif
