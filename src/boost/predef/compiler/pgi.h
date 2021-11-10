/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_PGI_H
#define BOOST_PREDEF_COMPILER_PGI_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_PGI`

http://en.wikipedia.org/wiki/The_Portland_Group[Portland Group C/{CPP}] compiler.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__PGI+` | {predef_detection}

| `+__PGIC__+`, `+__PGIC_MINOR__+`, `+__PGIC_PATCHLEVEL__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_PGI BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__PGI)
#   if !defined(BOOST_COMP_PGI_DETECTION) && (defined(__PGIC__) && defined(__PGIC_MINOR__) && defined(__PGIC_PATCHLEVEL__))
#       define BOOST_COMP_PGI_DETECTION BOOST_VERSION_NUMBER(__PGIC__,__PGIC_MINOR__,__PGIC_PATCHLEVEL__)
#   endif
#   if !defined(BOOST_COMP_PGI_DETECTION)
#       define BOOST_COMP_PGI_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_PGI_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_PGI_EMULATED BOOST_COMP_PGI_DETECTION
#   else
#       undef BOOST_COMP_PGI
#       define BOOST_COMP_PGI BOOST_COMP_PGI_DETECTION
#   endif
#   define BOOST_COMP_PGI_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_PGI_NAME "Portland Group C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PGI,BOOST_COMP_PGI_NAME)

#ifdef BOOST_COMP_PGI_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_PGI_EMULATED,BOOST_COMP_PGI_NAME)
#endif
