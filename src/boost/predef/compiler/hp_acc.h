/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_HP_ACC_H
#define BOOST_PREDEF_COMPILER_HP_ACC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_HPACC`

HP a{CPP} compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__HP_aCC+` | {predef_detection}

| `+__HP_aCC+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_HPACC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__HP_aCC)
#   if !defined(BOOST_COMP_HPACC_DETECTION) && (__HP_aCC > 1)
#       define BOOST_COMP_HPACC_DETECTION BOOST_PREDEF_MAKE_10_VVRRPP(__HP_aCC)
#   endif
#   if !defined(BOOST_COMP_HPACC_DETECTION)
#       define BOOST_COMP_HPACC_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_HPACC_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_HPACC_EMULATED BOOST_COMP_HPACC_DETECTION
#   else
#       undef BOOST_COMP_HPACC
#       define BOOST_COMP_HPACC BOOST_COMP_HPACC_DETECTION
#   endif
#   define BOOST_COMP_HPACC_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_HPACC_NAME "HP aC++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HPACC,BOOST_COMP_HPACC_NAME)

#ifdef BOOST_COMP_HPACC_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_HPACC_EMULATED,BOOST_COMP_HPACC_NAME)
#endif
