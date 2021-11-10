/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_PARISC_H
#define BOOST_PREDEF_ARCHITECTURE_PARISC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_PARISC`

http://en.wikipedia.org/wiki/PA-RISC_family[HP/PA RISC] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__hppa__+` | {predef_detection}
| `+__hppa+` | {predef_detection}
| `+__HPPA__+` | {predef_detection}

| `+_PA_RISC1_0+` | 1.0.0
| `+_PA_RISC1_1+` | 1.1.0
| `+__HPPA11__+` | 1.1.0
| `+__PA7100__+` | 1.1.0
| `+_PA_RISC2_0+` | 2.0.0
| `+__RISC2_0__+` | 2.0.0
| `+__HPPA20__+` | 2.0.0
| `+__PA8000__+` | 2.0.0
|===
*/ // end::reference[]

#define BOOST_ARCH_PARISC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__hppa__) || defined(__hppa) || defined(__HPPA__)
#   undef BOOST_ARCH_PARISC
#   if !defined(BOOST_ARCH_PARISC) && (defined(_PA_RISC1_0))
#       define BOOST_ARCH_PARISC BOOST_VERSION_NUMBER(1,0,0)
#   endif
#   if !defined(BOOST_ARCH_PARISC) && (defined(_PA_RISC1_1) || defined(__HPPA11__) || defined(__PA7100__))
#       define BOOST_ARCH_PARISC BOOST_VERSION_NUMBER(1,1,0)
#   endif
#   if !defined(BOOST_ARCH_PARISC) && (defined(_PA_RISC2_0) || defined(__RISC2_0__) || defined(__HPPA20__) || defined(__PA8000__))
#       define BOOST_ARCH_PARISC BOOST_VERSION_NUMBER(2,0,0)
#   endif
#   if !defined(BOOST_ARCH_PARISC)
#       define BOOST_ARCH_PARISC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_ARCH_PARISC
#   define BOOST_ARCH_PARISC_AVAILABLE
#endif

#if BOOST_ARCH_PARISC
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_PARISC_NAME "HP/PA RISC"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_PARISC,BOOST_ARCH_PARISC_NAME)
