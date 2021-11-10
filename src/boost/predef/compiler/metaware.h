/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_METAWARE_H
#define BOOST_PREDEF_COMPILER_METAWARE_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_HIGHC`

MetaWare High C/{CPP} compiler.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__HIGHC__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_COMP_HIGHC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__HIGHC__)
#   define BOOST_COMP_HIGHC_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif

#ifdef BOOST_COMP_HIGHC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_HIGHC_EMULATED BOOST_COMP_HIGHC_DETECTION
#   else
#       undef BOOST_COMP_HIGHC
#       define BOOST_COMP_HIGHC BOOST_COMP_HIGHC_DETECTION
#   endif
#   define BOOST_COMP_HIGHC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_HIGHC_NAME "MetaWare High C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HIGHC,BOOST_COMP_HIGHC_NAME)

#ifdef BOOST_COMP_HIGHC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HIGHC_EMULATED,BOOST_COMP_HIGHC_NAME)
#endif
