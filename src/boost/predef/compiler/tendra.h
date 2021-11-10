/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_TENDRA_H
#define BOOST_PREDEF_COMPILER_TENDRA_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_TENDRA`

http://en.wikipedia.org/wiki/TenDRA_Compiler[TenDRA C/{CPP}] compiler.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__TenDRA__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_COMP_TENDRA BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__TenDRA__)
#   define BOOST_COMP_TENDRA_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif

#ifdef BOOST_COMP_TENDRA_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_TENDRA_EMULATED BOOST_COMP_TENDRA_DETECTION
#   else
#       undef BOOST_COMP_TENDRA
#       define BOOST_COMP_TENDRA BOOST_COMP_TENDRA_DETECTION
#   endif
#   define BOOST_COMP_TENDRA_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_TENDRA_NAME "TenDRA C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_TENDRA,BOOST_COMP_TENDRA_NAME)

#ifdef BOOST_COMP_TENDRA_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_TENDRA_EMULATED,BOOST_COMP_TENDRA_NAME)
#endif
