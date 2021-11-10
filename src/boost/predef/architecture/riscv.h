/*
Copyright Andreas Schwab 2019
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_ARCHITECTURE_RISCV_H
#define BOOST_PREDEF_ARCHITECTURE_RISCV_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_ARCH_RISCV`

http://en.wikipedia.org/wiki/RISC-V[RISC-V] architecture.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__riscv+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_ARCH_RISCV BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__riscv)
#   undef BOOST_ARCH_RISCV
#   define BOOST_ARCH_RISCV BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_RISCV
#   define BOOST_ARCH_RISCV_AVAILABLE
#endif

#if BOOST_ARCH_RISCV
#   undef BOOST_ARCH_WORD_BITS_32
#   define BOOST_ARCH_WORD_BITS_32 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#define BOOST_ARCH_RISCV_NAME "RISC-V"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_RISCV,BOOST_ARCH_RISCV_NAME)
