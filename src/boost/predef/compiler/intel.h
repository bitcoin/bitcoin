/*
Copyright Rene Rivera 2008-2017
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_INTEL_H
#define BOOST_PREDEF_COMPILER_INTEL_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_INTEL`

http://en.wikipedia.org/wiki/Intel_C%2B%2B[Intel C/{CPP}] compiler.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__INTEL_COMPILER+` | {predef_detection}
| `+__ICL+` | {predef_detection}
| `+__ICC+` | {predef_detection}
| `+__ECC+` | {predef_detection}

| `+__INTEL_COMPILER+` | V.R
| `+__INTEL_COMPILER+` and `+__INTEL_COMPILER_UPDATE+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_COMP_INTEL BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || \
    defined(__ECC)
/* tag::reference[]
NOTE: Because of an Intel mistake in the release version numbering when
`__INTEL_COMPILER` is `9999` it is detected as version 12.1.0.
*/ // end::reference[]
#   if !defined(BOOST_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER) && (__INTEL_COMPILER == 9999)
#       define BOOST_COMP_INTEL_DETECTION BOOST_VERSION_NUMBER(12,1,0)
#   endif
#   if !defined(BOOST_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER) && defined(__INTEL_COMPILER_UPDATE)
#       define BOOST_COMP_INTEL_DETECTION BOOST_VERSION_NUMBER( \
            BOOST_VERSION_NUMBER_MAJOR(BOOST_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)), \
            BOOST_VERSION_NUMBER_MINOR(BOOST_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)), \
            __INTEL_COMPILER_UPDATE)
#   endif
#   if !defined(BOOST_COMP_INTEL_DETECTION) && defined(__INTEL_COMPILER)
#       define BOOST_COMP_INTEL_DETECTION BOOST_PREDEF_MAKE_10_VVRR(__INTEL_COMPILER)
#   endif
#   if !defined(BOOST_COMP_INTEL_DETECTION)
#       define BOOST_COMP_INTEL_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#ifdef BOOST_COMP_INTEL_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_INTEL_EMULATED BOOST_COMP_INTEL_DETECTION
#   else
#       undef BOOST_COMP_INTEL
#       define BOOST_COMP_INTEL BOOST_COMP_INTEL_DETECTION
#   endif
#   define BOOST_COMP_INTEL_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_INTEL_NAME "Intel C/C++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_INTEL,BOOST_COMP_INTEL_NAME)

#ifdef BOOST_COMP_INTEL_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_INTEL_EMULATED,BOOST_COMP_INTEL_NAME)
#endif
