/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_IBM_H
#define BOOST_PREDEF_COMPILER_IBM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_IBM`

http://en.wikipedia.org/wiki/VisualAge[IBM XL C/{CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__IBMCPP__+` | {predef_detection}
| `+__xlC__+` | {predef_detection}
| `+__xlc__+` | {predef_detection}

| `+__COMPILER_VER__+` | V.R.P
| `+__xlC__+` | V.R.P
| `+__xlc__+` | V.R.P
| `+__IBMCPP__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_IBM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__IBMCPP__) || defined(__xlC__) || defined(__xlc__)
#   if !defined(BOOST_COMP_IBM_DETECTION) && defined(__COMPILER_VER__)
#       define BOOST_COMP_IBM_DETECTION BOOST_PREDEF_MAKE_0X_VRRPPPP(__COMPILER_VER__)
#   endif
#   if !defined(BOOST_COMP_IBM_DETECTION) && defined(__xlC__)
#       define BOOST_COMP_IBM_DETECTION BOOST_PREDEF_MAKE_0X_VVRR(__xlC__)
#   endif
#   if !defined(BOOST_COMP_IBM_DETECTION) && defined(__xlc__)
#       define BOOST_COMP_IBM_DETECTION BOOST_PREDEF_MAKE_0X_VVRR(__xlc__)
#   endif
#   if !defined(BOOST_COMP_IBM_DETECTION)
#       define BOOST_COMP_IBM_DETECTION BOOST_PREDEF_MAKE_10_VRP(__IBMCPP__)
#   endif
#endif

#ifdef BOOST_COMP_IBM_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_IBM_EMULATED BOOST_COMP_IBM_DETECTION
#   else
#       undef BOOST_COMP_IBM
#       define BOOST_COMP_IBM BOOST_COMP_IBM_DETECTION
#   endif
#   define BOOST_COMP_IBM_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_IBM_NAME "IBM XL C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_IBM,BOOST_COMP_IBM_NAME)

#ifdef BOOST_COMP_IBM_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_IBM_EMULATED,BOOST_COMP_IBM_NAME)
#endif
