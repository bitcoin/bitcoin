/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_C_GNU_H
#define BOOST_PREDEF_LIBRARY_C_GNU_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

#include <boost/predef/library/c/_prefix.h>

#if defined(__STDC__)
#include <stddef.h>
#elif defined(__cplusplus)
#include <cstddef>
#endif

/* tag::reference[]
= `BOOST_LIB_C_GNU`

http://en.wikipedia.org/wiki/Glibc[GNU glibc] Standard C library.
Version number available as major, and minor.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__GLIBC__+` | {predef_detection}
| `+__GNU_LIBRARY__+` | {predef_detection}

| `+__GLIBC__+`, `+__GLIBC_MINOR__+` | V.R.0
| `+__GNU_LIBRARY__+`, `+__GNU_LIBRARY_MINOR__+` | V.R.0
|===
*/ // end::reference[]

#define BOOST_LIB_C_GNU BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
#   undef BOOST_LIB_C_GNU
#   if defined(__GLIBC__)
#       define BOOST_LIB_C_GNU \
            BOOST_VERSION_NUMBER(__GLIBC__,__GLIBC_MINOR__,0)
#   else
#       define BOOST_LIB_C_GNU \
            BOOST_VERSION_NUMBER(__GNU_LIBRARY__,__GNU_LIBRARY_MINOR__,0)
#   endif
#endif

#if BOOST_LIB_C_GNU
#   define BOOST_LIB_C_GNU_AVAILABLE
#endif

#define BOOST_LIB_C_GNU_NAME "GNU"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_C_GNU,BOOST_LIB_C_GNU_NAME)
