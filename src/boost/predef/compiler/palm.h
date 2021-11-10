/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_PALM_H
#define BOOST_PREDEF_COMPILER_PALM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_PALM`

Palm C/{CPP} compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+_PACC_VER+` | {predef_detection}

| `+_PACC_VER+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_PALM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(_PACC_VER)
#   define BOOST_COMP_PALM_DETECTION BOOST_PREDEF_MAKE_0X_VRRPP000(_PACC_VER)
#endif

#ifdef BOOST_COMP_PALM_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_PALM_EMULATED BOOST_COMP_PALM_DETECTION
#   else
#       undef BOOST_COMP_PALM
#       define BOOST_COMP_PALM BOOST_COMP_PALM_DETECTION
#   endif
#   define BOOST_COMP_PALM_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_PALM_NAME "Palm C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PALM,BOOST_COMP_PALM_NAME)

#ifdef BOOST_COMP_PALM_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PALM_EMULATED,BOOST_COMP_PALM_NAME)
#endif
