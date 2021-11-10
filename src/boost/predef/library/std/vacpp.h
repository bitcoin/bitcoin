/*
Copyright Rene Rivera 2008-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LIBRARY_STD_VACPP_H
#define BOOST_PREDEF_LIBRARY_STD_VACPP_H

#include <boost/predef/library/std/_prefix.h>

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LIB_STD_IBM`

http://www.ibm.com/software/awdtools/xlcpp/[IBM VACPP Standard {CPP}] library.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__IBMCPP__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_LIB_STD_IBM BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__IBMCPP__)
#   undef BOOST_LIB_STD_IBM
#   define BOOST_LIB_STD_IBM BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_LIB_STD_IBM
#   define BOOST_LIB_STD_IBM_AVAILABLE
#endif

#define BOOST_LIB_STD_IBM_NAME "IBM VACPP"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LIB_STD_IBM,BOOST_LIB_STD_IBM_NAME)
