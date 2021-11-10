/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#include <boost/predef/architecture/x86/32.h>
#include <boost/predef/architecture/x86/64.h>

#ifndef BOOST_PREDEF_ARCHITECTURE_X86_H
#define BOOST_PREDEF_ARCHITECTURE_X86_H

/* tag::reference[]
= `BOOST_ARCH_X86`

http://en.wikipedia.org/wiki/X86[Intel x86] architecture. This is
a category to indicate that either `BOOST_ARCH_X86_32` or
`BOOST_ARCH_X86_64` is detected.
*/ // end::reference[]

#define BOOST_ARCH_X86 BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if BOOST_ARCH_X86_32 || BOOST_ARCH_X86_64
#   undef BOOST_ARCH_X86
#   define BOOST_ARCH_X86 BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_ARCH_X86
#   define BOOST_ARCH_X86_AVAILABLE
#endif

#define BOOST_ARCH_X86_NAME "Intel x86"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_ARCH_X86,BOOST_ARCH_X86_NAME)
