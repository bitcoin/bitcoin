/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_BORLAND_H
#define BOOST_PREDEF_COMPILER_BORLAND_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_BORLAND`

http://en.wikipedia.org/wiki/C_plus_plus_builder[Borland {CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__BORLANDC__+` | {predef_detection}
| `+__CODEGEARC__+` | {predef_detection}

| `+__BORLANDC__+` | V.R.P
| `+__CODEGEARC__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_BORLAND BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__BORLANDC__) || defined(__CODEGEARC__)
#   if !defined(BOOST_COMP_BORLAND_DETECTION) && (defined(__CODEGEARC__))
#       define BOOST_COMP_BORLAND_DETECTION BOOST_PREDEF_MAKE_0X_VVRP(__CODEGEARC__)
#   endif
#   if !defined(BOOST_COMP_BORLAND_DETECTION)
#       define BOOST_COMP_BORLAND_DETECTION BOOST_PREDEF_MAKE_0X_VVRP(__BORLANDC__)
#   endif
#endif

#ifdef BOOST_COMP_BORLAND_DETECTION
#   define BOOST_COMP_BORLAND_AVAILABLE
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_BORLAND_EMULATED BOOST_COMP_BORLAND_DETECTION
#   else
#       undef BOOST_COMP_BORLAND
#       define BOOST_COMP_BORLAND BOOST_COMP_BORLAND_DETECTION
#   endif
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_BORLAND_NAME "Borland C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_BORLAND,BOOST_COMP_BORLAND_NAME)

#ifdef BOOST_COMP_BORLAND_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_BORLAND_EMULATED,BOOST_COMP_BORLAND_NAME)
#endif
