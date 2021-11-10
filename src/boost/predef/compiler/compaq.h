/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_COMPAQ_H
#define BOOST_PREDEF_COMPILER_COMPAQ_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_DEC`

http://www.openvms.compaq.com/openvms/brochures/deccplus/[Compaq C/{CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__DECCXX+` | {predef_detection}
| `+__DECC+` | {predef_detection}

| `+__DECCXX_VER+` | V.R.P
| `+__DECC_VER+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_DEC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__DECC) || defined(__DECCXX)
#   if !defined(BOOST_COMP_DEC_DETECTION) && defined(__DECCXX_VER)
#       define BOOST_COMP_DEC_DETECTION BOOST_PREDEF_MAKE_10_VVRR0PP00(__DECCXX_VER)
#   endif
#   if !defined(BOOST_COMP_DEC_DETECTION) && defined(__DECC_VER)
#       define BOOST_COMP_DEC_DETECTION BOOST_PREDEF_MAKE_10_VVRR0PP00(__DECC_VER)
#   endif
#   if !defined(BOOST_COMP_DEC_DETECTION)
#       define BOOST_COMP_DEC_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_DEC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_DEC_EMULATED BOOST_COMP_DEC_DETECTION
#   else
#       undef BOOST_COMP_DEC
#       define BOOST_COMP_DEC BOOST_COMP_DEC_DETECTION
#   endif
#   define BOOST_COMP_DEC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_DEC_NAME "Compaq C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DEC,BOOST_COMP_DEC_NAME)

#ifdef BOOST_COMP_DEC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_DEC_EMULATED,BOOST_COMP_DEC_NAME)
#endif
