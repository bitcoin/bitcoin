/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_MIPS_H
#define BOOST_PREDEF_ARCHITECTURE_MIPS_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_MIPS`

http://en.wikipedia.org/wiki/MIPS_architecture[MIPS] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__mips__+` | {predef_detection}
| `+__mips+` | {predef_detection}
| `+__MIPS__+` | {predef_detection}

| `+__mips+` | V.0.0
| `+_MIPS_ISA_MIPS1+` | 1.0.0
| `+_R3000+` | 1.0.0
| `+_MIPS_ISA_MIPS2+` | 2.0.0
| `+__MIPS_ISA2__+` | 2.0.0
| `+_R4000+` | 2.0.0
| `+_MIPS_ISA_MIPS3+` | 3.0.0
| `+__MIPS_ISA3__+` | 3.0.0
| `+_MIPS_ISA_MIPS4+` | 4.0.0
| `+__MIPS_ISA4__+` | 4.0.0
|===
*/ // end::reference[]

#define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__mips__) || defined(__mips) || \
    defined(__MIPS__)
#   undef BOOST_ARCH_MIPS
#   if !defined(BOOST_ARCH_MIPS) && (defined(__mips))
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER(__mips,0,0)
#   endif
#   if !defined(BOOST_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS1) || defined(_R3000))
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER(1,0,0)
#   endif
#   if !defined(BOOST_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS2) || defined(__MIPS_ISA2__) || defined(_R4000))
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS3) || defined(__MIPS_ISA3__))
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER(3,0,0)
#   endif
#   if !defined(BOOST_ARCH_MIPS) && (defined(_MIPS_ISA_MIPS4) || defined(__MIPS_ISA4__))
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER(4,0,0)
#   endif
#   if !defined(BOOST_ARCH_MIPS)
#       define BOOST_ARCH_MIPS BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_ARCH_MIPS
#   define BOOST_ARCH_MIPS_AVAILABLE
#endif

#if BOOST_ARCH_MIPS
#   if BOOST_ARCH_MIPS >= BOOST_VERSION_NUMBER(3,0,0)
#       undef BOOST_ARCH_WORD_BITS_64
#       define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_AVAILABLE
#   else
#       undef BOOST_ARCH_WORD_BITS_32
#       define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#define BOOST_ARCH_MIPS_NAME "MIPS"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_MIPS,BOOST_ARCH_MIPS_NAME)
