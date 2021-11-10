/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_COMEAU_H
#define BOOST_PREDEF_COMPILER_COMEAU_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

#define BOOST_COMP_COMO BOOST_VERSION_NUMBER_NOT_AVAILABLE

/* tag::reference[]
= `BOOST_COMP_COMO`

http://en.wikipedia.org/wiki/Comeau_C/C%2B%2B[Comeau {CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__COMO__+` | {predef_detection}

| `+__COMO_VERSION__+` | V.R.P
|===
*/ // end::reference[]

#if defined(__COMO__)
#   if !defined(BOOST_COMP_COMO_DETECTION) && defined(__COMO_VERSION__)
#       define BOOST_COMP_COMO_DETECTION BOOST_PREDEF_MAKE_0X_VRP(__COMO_VERSION__)
#   endif
#   if !defined(BOOST_COMP_COMO_DETECTION)
#       define BOOST_COMP_COMO_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_COMO_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_COMO_EMULATED BOOST_COMP_COMO_DETECTION
#   else
#       undef BOOST_COMP_COMO
#       define BOOST_COMP_COMO BOOST_COMP_COMO_DETECTION
#   endif
#   define BOOST_COMP_COMO_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_COMO_NAME "Comeau C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_COMO,BOOST_COMP_COMO_NAME)

#ifdef BOOST_COMP_COMO_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_COMO_EMULATED,BOOST_COMP_COMO_NAME)
#endif
