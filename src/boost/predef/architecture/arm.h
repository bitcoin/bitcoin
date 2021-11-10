/*
Copyright Rene Rivera 2008-2019
Copyright Franz Detro 2014
Copyright (c) Microsoft Corporation 2014
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_ARM_H
#define BOOST_PREDEF_ARCHITECTURE_ARM_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_ARM`

http://en.wikipedia.org/wiki/ARM_architecture[ARM] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__ARM_ARCH+` | {predef_detection}
| `+__TARGET_ARCH_ARM+` | {predef_detection}
| `+__TARGET_ARCH_THUMB+` | {predef_detection}
| `+_M_ARM+` | {predef_detection}
| `+__arm__+` | {predef_detection}
| `+__arm64+` | {predef_detection}
| `+__thumb__+` | {predef_detection}
| `+_M_ARM64+` | {predef_detection}
| `+__aarch64__+` | {predef_detection}
| `+__AARCH64EL__+` | {predef_detection}
| `+__ARM_ARCH_7__+` | {predef_detection}
| `+__ARM_ARCH_7A__+` | {predef_detection}
| `+__ARM_ARCH_7R__+` | {predef_detection}
| `+__ARM_ARCH_7M__+` | {predef_detection}
| `+__ARM_ARCH_6K__+` | {predef_detection}
| `+__ARM_ARCH_6Z__+` | {predef_detection}
| `+__ARM_ARCH_6KZ__+` | {predef_detection}
| `+__ARM_ARCH_6T2__+` | {predef_detection}
| `+__ARM_ARCH_5TE__+` | {predef_detection}
| `+__ARM_ARCH_5TEJ__+` | {predef_detection}
| `+__ARM_ARCH_4T__+` | {predef_detection}
| `+__ARM_ARCH_4__+` | {predef_detection}

| `+__ARM_ARCH+` | V.0.0
| `+__TARGET_ARCH_ARM+` | V.0.0
| `+__TARGET_ARCH_THUMB+` | V.0.0
| `+_M_ARM+` | V.0.0
| `+__arm64+` | 8.0.0
| `+_M_ARM64+` | 8.0.0
| `+__aarch64__+` | 8.0.0
| `+__AARCH64EL__+` | 8.0.0
| `+__ARM_ARCH_7__+` | 7.0.0
| `+__ARM_ARCH_7A__+` | 7.0.0
| `+__ARM_ARCH_7R__+` | 7.0.0
| `+__ARM_ARCH_7M__+` | 7.0.0
| `+__ARM_ARCH_6K__+` | 6.0.0
| `+__ARM_ARCH_6Z__+` | 6.0.0
| `+__ARM_ARCH_6KZ__+` | 6.0.0
| `+__ARM_ARCH_6T2__+` | 6.0.0
| `+__ARM_ARCH_5TE__+` | 5.0.0
| `+__ARM_ARCH_5TEJ__+` | 5.0.0
| `+__ARM_ARCH_4T__+` | 4.0.0
| `+__ARM_ARCH_4__+` | 4.0.0
|===
*/ // end::reference[]

#define BOOST_ARCH_ARM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if \
    defined(__ARM_ARCH) || defined(__TARGET_ARCH_ARM) || \
    defined(__TARGET_ARCH_THUMB) || defined(_M_ARM) || \
    defined(__arm__) || defined(__arm64) || defined(__thumb__) || \
    defined(_M_ARM64) || defined(__aarch64__) || defined(__AARCH64EL__) || \
    defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || \
    defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || \
    defined(__ARM_ARCH_6KZ__) || defined(__ARM_ARCH_6T2__) || \
    defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) || \
    defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__)
#   undef BOOST_ARCH_ARM
#   if !defined(BOOST_ARCH_ARM) && defined(__ARM_ARCH)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(__ARM_ARCH,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && defined(__TARGET_ARCH_ARM)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(__TARGET_ARCH_ARM,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && defined(__TARGET_ARCH_THUMB)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(__TARGET_ARCH_THUMB,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && defined(_M_ARM)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(_M_ARM,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && ( \
        defined(__arm64) || defined(_M_ARM64) || defined(__aarch64__) || \
        defined(__AARCH64EL__) )
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(8,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && ( \
    defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || \
    defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) )
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(7,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && ( \
    defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || \
    defined(__ARM_ARCH_6KZ__) || defined(__ARM_ARCH_6T2__) )
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(6,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && ( \
    defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) )
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(5,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM) && ( \
    defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__) )
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER(4,0,0)
#   endif
#   if !defined(BOOST_ARCH_ARM)
#       define BOOST_ARCH_ARM BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_ARCH_ARM
#   define BOOST_ARCH_ARM_AVAILABLE
#endif

#if BOOST_ARCH_ARM
#   if BOOST_ARCH_ARM >= BOOST_VERSION_NUMBER(8,0,0)
#       undef BOOST_ARCH_WORD_BITS_64
#       define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#   else
#       undef BOOST_ARCH_WORD_BITS_32
#       define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#define BOOST_ARCH_ARM_NAME "ARM"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_ARM,BOOST_ARCH_ARM_NAME)
