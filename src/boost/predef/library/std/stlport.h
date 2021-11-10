/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_STD_STLPORT_H
#define BOOST_PREDEF_LIBRARY_STD_STLPORT_H

#include <boost/predef/library/std/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_STD_STLPORT`

http://sourceforge.net/projects/stlport/[STLport Standard {CPP}] library.
Version number available as major, minor, and patch.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__SGI_STL_PORT+` | {predef_detection}
| `+_STLPORT_VERSION+` | {predef_detection}

| `+_STLPORT_MAJOR+`, `+_STLPORT_MINOR+`, `+_STLPORT_PATCHLEVEL+` | V.R.P
| `+_STLPORT_VERSION+` | V.R.P
| `+__SGI_STL_PORT+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_LIB_STD_STLPORT BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__SGI_STL_PORT) || defined(_STLPORT_VERSION)
#   undef BOOST_LIB_STD_STLPORT
#   if !defined(BOOST_LIB_STD_STLPORT) && defined(_STLPORT_MAJOR)
#       define BOOST_LIB_STD_STLPORT \
            BOOST_VERSION_NUMBER(_STLPORT_MAJOR,_STLPORT_MINOR,_STLPORT_PATCHLEVEL)
#   endif
#   if !defined(BOOST_LIB_STD_STLPORT) && defined(_STLPORT_VERSION)
#       define BOOST_LIB_STD_STLPORT BOOST_PREDEF_MAKE_0X_VRP(_STLPORT_VERSION)
#   endif
#   if !defined(BOOST_LIB_STD_STLPORT)
#       define BOOST_LIB_STD_STLPORT BOOST_PREDEF_MAKE_0X_VRP(__SGI_STL_PORT)
#   endif
#endif

#if BOOST_LIB_STD_STLPORT
#   define BOOST_LIB_STD_STLPORT_AVAILABLE
#endif

#define BOOST_LIB_STD_STLPORT_NAME "STLport"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_STLPORT,BOOST_LIB_STD_STLPORT_NAME)
