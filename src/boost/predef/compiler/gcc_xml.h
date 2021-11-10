/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_COMPILER_GCC_XML_H
#define BOOST_PREDEF_COMPILER_GCC_XML_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_COMP_GCCXML`

http://www.gccxml.org/[GCC XML] compiler.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__GCCXML__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_COMP_GCCXML BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__GCCXML__)
#   define BOOST_COMP_GCCXML_DETECTION BOOST_VERSION_NUMBER_AVAILABLE
#endif

#ifdef BOOST_COMP_GCCXML_DETECTION
#   if defined(BOOST_PREDEF_DETAIL_COMP_DETECTED)
#       define BOOST_COMP_GCCXML_EMULATED BOOST_COMP_GCCXML_DETECTION
#   else
#       undef BOOST_COMP_GCCXML
#       define BOOST_COMP_GCCXML BOOST_COMP_GCCXML_DETECTION
#   endif
#   define BOOST_COMP_GCCXML_AVAILABLE
#   include <boost/predef/detail/comp_detected.h>
#endif

#define BOOST_COMP_GCCXML_NAME "GCC XML"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GCCXML,BOOST_COMP_GCCXML_NAME)

#ifdef BOOST_COMP_GCCXML_EMULATED
#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_COMP_GCCXML_EMULATED,BOOST_COMP_GCCXML_NAME)
#endif
