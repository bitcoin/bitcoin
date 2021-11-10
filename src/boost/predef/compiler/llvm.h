/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_LLVM_H
#define BOOST_PREDEF_COMPILER_LLVM_H

/* Other compilers that emulate this one need to be detected first. */

#include <boost/predef/compiler/clang.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_LLVM`

http://en.wikipedia.org/wiki/LLVM[LLVM] compiler.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__llvm__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_COMP_LLVM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__llvm__)
#   define BOOST_COMP_LLVM_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif

#ifdef BOOST_COMP_LLVM_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_LLVM_EMULATED BOOST_COMP_LLVM_DETECTION
#   else
#       undef BOOST_COMP_LLVM
#       define BOOST_COMP_LLVM BOOST_COMP_LLVM_DETECTION
#   endif
#   define BOOST_COMP_LLVM_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_LLVM_NAME "LLVM"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_LLVM,BOOST_COMP_LLVM_NAME)

#ifdef BOOST_COMP_LLVM_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_LLVM_EMULATED,BOOST_COMP_LLVM_NAME)
#endif
