/*
Copyright Rene Rivera 2011-2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_LANGUAGE_STDC_H
#define BOOST_PREDEF_LANGUAGE_STDC_H

#include <boost/predef/version_number.h>
#include <boost/predef/make.h>

/* tag::reference[]
= `BOOST_LANG_STDC`

http://en.wikipedia.org/wiki/C_(programming_language)[Standard C] language.
If available, the year of the standard is detected as YYYY.MM.1 from the Epoc date.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `+__STDC__+` | {predef_detection}

| `+__STDC_VERSION__+` | V.R.P
|===
*/ // end::reference[]

#define BOOST_LANG_STDC BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if defined(__STDC__)
#   undef BOOST_LANG_STDC
#   if defined(__STDC_VERSION__)
#       if (__STDC_VERSION__ > 100)
#           define BOOST_LANG_STDC BOOST_PREDEF_MAKE_YYYYMM(__STDC_VERSION__)
#       else
#           define BOOST_LANG_STDC BOOST_VERSION_NUMBER_AVAILABLE
#       endif
#   else
#       define BOOST_LANG_STDC BOOST_VERSION_NUMBER_AVAILABLE
#   endif
#endif

#if BOOST_LANG_STDC
#   define BOOST_LANG_STDC_AVAILABLE
#endif

#define BOOST_LANG_STDC_NAME "Standard C"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_LANG_STDC,BOOST_LANG_STDC_NAME)
