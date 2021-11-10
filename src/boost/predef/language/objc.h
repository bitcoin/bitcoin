/*
Copyright Rene Rivera 2011-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LANGUAGE_OBJC_H
#define BOOST_PREDEF_LANGUAGE_OBJC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LANG_OBJC`

http://en.wikipedia.org/wiki/Objective-C[Objective-C] language.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__OBJC__+` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_LANG_OBJC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__OBJC__)
#   undef BOOST_LANG_OBJC
#   define BOOST_LANG_OBJC BOOST_VERSION_NUMBER_AVAILABLE
#endif

#if BOOST_LANG_OBJC
#   define BOOST_LANG_OBJC_AVAILABLE
#endif

#define BOOST_LANG_OBJC_NAME "Objective-C"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_OBJC,BOOST_LANG_OBJC_NAME)
