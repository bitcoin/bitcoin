/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_M68K_H
#define BOOST_PREDEF_ARCHITECTURE_M68K_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_M68K`

http://en.wikipedia.org/wiki/M68k[Motorola 68k] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__m68k__+` | {predef_detection}
| `M68000` | {predef_detection}

| `+__mc68060__+` | 6.0.0
| `mc68060` | 6.0.0
| `+__mc68060+` | 6.0.0
| `+__mc68040__+` | 4.0.0
| `mc68040` | 4.0.0
| `+__mc68040+` | 4.0.0
| `+__mc68030__+` | 3.0.0
| `mc68030` | 3.0.0
| `+__mc68030+` | 3.0.0
| `+__mc68020__+` | 2.0.0
| `mc68020` | 2.0.0
| `+__mc68020+` | 2.0.0
| `+__mc68010__+` | 1.0.0
| `mc68010` | 1.0.0
| `+__mc68010+` | 1.0.0
| `+__mc68000__+` | 0.0.1
| `mc68000` | 0.0.1
| `+__mc68000+` | 0.0.1
|===
*/ // end::reference[]

#define BOOST_ARCH_M68K BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__m68k__) || defined(M68000)
#   undef BOOST_ARCH_M68K
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68060__) || defined(mc68060) || defined(__mc68060))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER(6,0,0)
#   endif
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68040__) || defined(mc68040) || defined(__mc68040))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER(4,0,0)
#   endif
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68030__) || defined(mc68030) || defined(__mc68030))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER(3,0,0)
#   endif
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68020__) || defined(mc68020) || defined(__mc68020))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68010__) || defined(mc68010) || defined(__mc68010))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER(1,0,0)
#   endif
#   if !defined(BOOST_ARCH_M68K) && (defined(__mc68000__) || defined(mc68000) || defined(__mc68000))
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#   if !defined(BOOST_ARCH_M68K)
#       define BOOST_ARCH_M68K BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_ARCH_M68K
#   define BOOST_ARCH_M68K_AVAILABLE
#endif

#if BOOST_ARCH_M68K
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_M68K_NAME "Motorola 68k"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_M68K,BOOST_ARCH_M68K_NAME)
