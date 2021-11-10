/*
Copyright Rene Ferdinand Rivera Morell 2020-2021
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_OTHER_WORD_SIZE_H
#define BOOST_PREDEF_OTHER_WORD_SIZE_H

#include <boost/predef/architecture.h>
#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_WORD_BITS`

Detects the native word size, in bits, for the current architecture. There are
two types of macros for this detection:

* `BOOST_ARCH_WORD_BITS`, gives the number of word size bits
  (16, 32, 64).
* `BOOST_ARCH_WORD_BITS_16`, `BOOST_ARCH_WORD_BITS_32`, and
  `BOOST_ARCH_WORD_BITS_64`, indicate when the given word size is
  detected.

They allow for both single checks and direct use of the size in code.

NOTE: The word size is determined manually on each architecture. Hence use of
the `wordsize.h` header will also include all the architecture headers.

*/ // end::reference[]

#if !defined(BOOST_ARCH_WORD_BITS_64)
#   define BOOST_ARCH_WORD_BITS_64 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#elif !defined(BOOST_ARCH_WORD_BITS)
#   define BOOST_ARCH_WORD_BITS 64
#endif

#if !defined(BOOST_ARCH_WORD_BITS_32)
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#elif !defined(BOOST_ARCH_WORD_BITS)
#	  define BOOST_ARCH_WORD_BITS 32
#endif

#if !defined(BOOST_ARCH_WORD_BITS_16)
#   define BOOST_ARCH_WORD_BITS_16 BOOST_VERSION_NUMBER_NOT_AVAILABLE
#elif !defined(BOOST_ARCH_WORD_BITS)
#   define BOOST_ARCH_WORD_BITS 16
#endif

#if !defined(BOOST_ARCH_WORD_BITS)
#   define BOOST_ARCH_WORD_BITS 0
#endif

#define BOOST_ARCH_WORD_BITS_NAME "Word Bits"
#define BOOST_ARCH_WORD_BITS_16_NAME "16-bit Word Size"
#define BOOST_ARCH_WORD_BITS_32_NAME "32-bit Word Size"
#define BOOST_ARCH_WORD_BITS_64_NAME "64-bit Word Size"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_WORD_BITS,BOOST_ARCH_WORD_BITS_NAME)

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_WORD_BITS_16,BOOST_ARCH_WORD_BITS_16_NAME)

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_WORD_BITS_32,BOOST_ARCH_WORD_BITS_32_NAME)

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_WORD_BITS_64,BOOST_ARCH_WORD_BITS_64_NAME)
