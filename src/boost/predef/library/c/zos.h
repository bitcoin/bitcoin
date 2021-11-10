/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_C_ZOS_H
#define BOOST_PREDEF_LIBRARY_C_ZOS_H

#include <boost/predef/library/c/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_C_ZOS`

z/OS libc Standard C library.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__LIBREL__+` | {predef_detection}

| `+__LIBREL__+` | V.R.P
| `+__TARGET_LIB__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_LIB_C_ZOS BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__LIBREL__)
#   undef BOOST_LIB_C_ZOS
#   if !defined(BOOST_LIB_C_ZOS) && defined(__LIBREL__)
#       define BOOST_LIB_C_ZOS BOOST_PREDEF_MAKE_0X_VRRPPPP(__LIBREL__)
#   endif
#   if !defined(BOOST_LIB_C_ZOS) && defined(__TARGET_LIB__)
#       define BOOST_LIB_C_ZOS BOOST_PREDEF_MAKE_0X_VRRPPPP(__TARGET_LIB__)
#   endif
#   if !defined(BOOST_LIB_C_ZOS)
#       define BOOST_LIB_C_ZOS BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_LIB_C_ZOS
#   define BOOST_LIB_C_ZOS_AVAILABLE
#endif

#define BOOST_LIB_C_ZOS_NAME "z/OS"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_C_ZOS,BOOST_LIB_C_ZOS_NAME)
