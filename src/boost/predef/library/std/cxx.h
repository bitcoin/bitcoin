/*
Copyright Rene Rivera 2011-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_STD_CXX_H
#define BOOST_PREDEF_LIBRARY_STD_CXX_H

#include <boost/predef/library/std/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_STD_CXX`

http://libcxx.llvm.org/[libc++] {CPP} Standard Library.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+_LIBCPP_VERSION+` | {predef_detection}

| `+_LIBCPP_VERSION+` | V.0.P
|===
*/ // end::reference[]

#define BOOST_LIB_STD_CXX BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(_LIBCPP_VERSION)
#   undef BOOST_LIB_STD_CXX
#   define BOOST_LIB_STD_CXX BOOST_PREDEF_MAKE_10_VVPPP(_LIBCPP_VERSION)
#endif

#if BOOST_LIB_STD_CXX
#   define BOOST_LIB_STD_CXX_AVAILABLE
#endif

#define BOOST_LIB_STD_CXX_NAME "libc++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_CXX,BOOST_LIB_STD_CXX_NAME)
