/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_DIGITALMARS_H
#define BOOST_PREDEF_COMPILER_DIGITALMARS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_DMC`

http://en.wikipedia.org/wiki/Digital_Mars[Digital Mars] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__DMC__+` | {predef_detection}

| `+__DMC__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_DMC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__DMC__)
#   define BOOST_COMP_DMC_DETECTION BOOST_PREDEF_MAKE_0X_VRP(__DMC__)
#endif

#ifdef BOOST_COMP_DMC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_DMC_EMULATED BOOST_COMP_DMC_DETECTION
#   else
#       undef BOOST_COMP_DMC
#       define BOOST_COMP_DMC BOOST_COMP_DMC_DETECTION
#   endif
#   define BOOST_COMP_DMC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_DMC_NAME "Digital Mars"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DMC,BOOST_COMP_DMC_NAME)

#ifdef BOOST_COMP_DMC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DMC_EMULATED,BOOST_COMP_DMC_NAME)
#endif
