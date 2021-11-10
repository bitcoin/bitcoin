/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_EKOPATH_H
#define BOOST_PREDEF_COMPILER_EKOPATH_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_PATH`

http://en.wikipedia.org/wiki/PathScale[EKOpath] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__PATHCC__+` | {predef_detection}

| `+__PATHCC__+`, `+__PATHCC_MINOR__+`, `+__PATHCC_PATCHLEVEL__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_PATH BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__PATHCC__)
#   define BOOST_COMP_PATH_DETECTION \
        BOOST_VERSION_NUMBER(__PATHCC__,__PATHCC_MINOR__,__PATHCC_PATCHLEVEL__)
#endif

#ifdef BOOST_COMP_PATH_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_PATH_EMULATED BOOST_COMP_PATH_DETECTION
#   else
#       undef BOOST_COMP_PATH
#       define BOOST_COMP_PATH BOOST_COMP_PATH_DETECTION
#   endif
#   define BOOST_COMP_PATH_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_PATH_NAME "EKOpath"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PATH,BOOST_COMP_PATH_NAME)

#ifdef BOOST_COMP_PATH_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PATH_EMULATED,BOOST_COMP_PATH_NAME)
#endif
