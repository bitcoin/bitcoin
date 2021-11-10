/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_STD_MODENA_H
#define BOOST_PREDEF_LIBRARY_STD_MODENA_H

#include <boost/predef/library/std/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_STD_MSIPL`

http://modena.us/[Modena Software Lib++] Standard {CPP} Library.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `MSIPL_COMPILE_H` | {predef_detection}
| `+__MSIPL_COMPILE_H+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_LIB_STD_MSIPL BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(MSIPL_COMPILE_H) || defined(__MSIPL_COMPILE_H)
#   undef BOOST_LIB_STD_MSIPL
#   define BOOST_LIB_STD_MSIPL BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_LIB_STD_MSIPL
#   define BOOST_LIB_STD_MSIPL_AVAILABLE
#endif

#define BOOST_LIB_STD_MSIPL_NAME "Modena Software Lib++"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_MSIPL,BOOST_LIB_STD_MSIPL_NAME)
