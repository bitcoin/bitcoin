/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_SGI_MIPSPRO_H
#define BOOST_PREDEF_COMPILER_SGI_MIPSPRO_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_SGI`

http://en.wikipedia.org/wiki/MIPSpro[SGI MIPSpro] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__sgi+` | {predef_detection}
| `sgi` | {predef_detection}

| `+_SGI_COMPILER_VERSION+` | V.R.P
| `+_COMPILER_VERSION+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_SGI BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__sgi) || defined(sgi)
#   if !defined(BOOST_COMP_SGI_DETECTION) && defined(_SGI_COMPILER_VERSION)
#       define BOOST_COMP_SGI_DETECTION BOOST_PREDEF_MAKE_10_VRP(_SGI_COMPILER_VERSION)
#   endif
#   if !defined(BOOST_COMP_SGI_DETECTION) && defined(_COMPILER_VERSION)
#       define BOOST_COMP_SGI_DETECTION BOOST_PREDEF_MAKE_10_VRP(_COMPILER_VERSION)
#   endif
#   if !defined(BOOST_COMP_SGI_DETECTION)
#       define BOOST_COMP_SGI_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_SGI_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_SGI_EMULATED BOOST_COMP_SGI_DETECTION
#   else
#       undef BOOST_COMP_SGI
#       define BOOST_COMP_SGI BOOST_COMP_SGI_DETECTION
#   endif
#   define BOOST_COMP_SGI_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_SGI_NAME "SGI MIPSpro"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SGI,BOOST_COMP_SGI_NAME)

#ifdef BOOST_COMP_SGI_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SGI_EMULATED,BOOST_COMP_SGI_NAME)
#endif
