/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_DIGNUS_H
#define BOOST_PREDEF_COMPILER_DIGNUS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_SYSC`

http://www.dignus.com/dcxx/[Dignus Systems/{CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__SYSC__+` | {predef_detection}

| `+__SYSC_VER__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_SYSC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__SYSC__)
#   define BOOST_COMP_SYSC_DETECTION BOOST_PREDEF_MAKE_10_VRRPP(__SYSC_VER__)
#endif

#ifdef BOOST_COMP_SYSC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_SYSC_EMULATED BOOST_COMP_SYSC_DETECTION
#   else
#       undef BOOST_COMP_SYSC
#       define BOOST_COMP_SYSC BOOST_COMP_SYSC_DETECTION
#   endif
#   define BOOST_COMP_SYSC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_SYSC_NAME "Dignus Systems/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SYSC,BOOST_COMP_SYSC_NAME)

#ifdef BOOST_COMP_SYSC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_SYSC_EMULATED,BOOST_COMP_SYSC_NAME)
#endif
