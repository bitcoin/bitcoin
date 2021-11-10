/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_STD_ROGUEWAVE_H
#define BOOST_PREDEF_LIBRARY_STD_ROGUEWAVE_H

#include <boost/predef/library/std/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_STD_RW`

http://stdcxx.apache.org/[Roguewave] Standard {CPP} library.
If available version number as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__STD_RWCOMPILER_H__+` | {predef_detection}
| `+_RWSTD_VER+` | {predef_detection}

| `+_RWSTD_VER+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_LIB_STD_RW BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__STD_RWCOMPILER_H__) || defined(_RWSTD_VER)
#   undef BOOST_LIB_STD_RW
#   if defined(_RWSTD_VER)
#       if _RWSTD_VER < 0x010000
#           define BOOST_LIB_STD_RW BOOST_PREDEF_MAKE_0X_VVRRP(_RWSTD_VER)
#       else
#           define BOOST_LIB_STD_RW BOOST_PREDEF_MAKE_0X_VVRRPP(_RWSTD_VER)
#       endif
#   else
#       define BOOST_LIB_STD_RW BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_LIB_STD_RW
#   define BOOST_LIB_STD_RW_AVAILABLE
#endif

#define BOOST_LIB_STD_RW_NAME "Roguewave"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_RW,BOOST_LIB_STD_RW_NAME)
