/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_CLANG_H
#define BOOST_PREDEF_COMPILER_CLANG_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_CLANG`

http://en.wikipedia.org/wiki/Clang[Clang] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__clang__+` | {predef_detection}

| `+__clang_major__+`, `+__clang_minor__+`, `+__clang_patchlevel__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_CLANG BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__clang__)
#   define BOOST_COMP_CLANG_DETECTION BOOST_VERSION_NUMBER(__clang_major__,__clang_minor__,__clang_patchlevel__)
#endif

#ifdef BOOST_COMP_CLANG_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_CLANG_EMULATED BOOST_COMP_CLANG_DETECTION
#   else
#       undef BOOST_COMP_CLANG
#       define BOOST_COMP_CLANG BOOST_COMP_CLANG_DETECTION
#   endif
#   define BOOST_COMP_CLANG_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_CLANG_NAME "Clang"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_CLANG,BOOST_COMP_CLANG_NAME)

#ifdef BOOST_COMP_CLANG_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_CLANG_EMULATED,BOOST_COMP_CLANG_NAME)
#endif
