/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_GCC_H
#define BOOST_PREDEF_COMPILER_GCC_H

/* Other compilers that emulate this one need to be detected first. */

#include <boost/predef/compiler/clang.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_GNUC`

http://en.wikipedia.org/wiki/GNU_Compiler_Collection[Gnu GCC C/{CPP}] compiler.
Version number available as major, minor, and patch (if available).

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__GNUC__+` | {predef_detection}

| `+__GNUC__+`, `+__GNUC_MINOR__+`, `+__GNUC_PATCHLEVEL__+` | V.R.P
| `+__GNUC__+`, `+__GNUC_MINOR__+` | V.R.0
|===
*/ // end::reference[]

#define BOOST_COMP_GNUC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__GNUC__)
#   if !defined(BOOST_COMP_GNUC_DETECTION) && defined(__GNUC_PATCHLEVEL__)
#       define BOOST_COMP_GNUC_DETECTION \
            BOOST_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,__GNUC_PATCHLEVEL__)
#   endif
#   if !defined(BOOST_COMP_GNUC_DETECTION)
#       define BOOST_COMP_GNUC_DETECTION \
            BOOST_VERSION_NUMBER(__GNUC__,__GNUC_MINOR__,0)
#   endif
#endif

#ifdef BOOST_COMP_GNUC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_GNUC_EMULATED BOOST_COMP_GNUC_DETECTION
#   else
#       undef BOOST_COMP_GNUC
#       define BOOST_COMP_GNUC BOOST_COMP_GNUC_DETECTION
#   endif
#   define BOOST_COMP_GNUC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_GNUC_NAME "Gnu GCC C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GNUC,BOOST_COMP_GNUC_NAME)

#ifdef BOOST_COMP_GNUC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GNUC_EMULATED,BOOST_COMP_GNUC_NAME)
#endif
