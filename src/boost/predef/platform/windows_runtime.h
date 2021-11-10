/*
Copyright (c) Microsoft Corporation 2014
Copyright Rene Rivera 2015
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_PREDEF_PLAT_WINDOWS_RUNTIME_H
#define BOOST_PREDEF_PLAT_WINDOWS_RUNTIME_H

#include <boost/predef/make.h>
#include <boost/predef/os/windows.h>
#include <boost/predef/platform/windows_phone.h>
#include <boost/predef/platform/windows_store.h>
#include <boost/predef/version_number.h>

/* tag::reference[]
= `BOOST_PLAT_WINDOWS_RUNTIME`

Deprecated.

https://docs.microsoft.com/en-us/windows/uwp/get-started/universal-application-platform-guide[UWP]
for Windows Phone or Store development.  This does not align to the existing development model for
UWP and is deprecated.  Use one of the other `BOOST_PLAT_WINDOWS_*`definitions instead.

[options="header"]
|===
| {predef_symbol} | {predef_version}

| `BOOST_PLAT_WINDOWS_PHONE` | {predef_detection}
| `BOOST_PLAT_WINDOWS_STORE` | {predef_detection}
|===
*/ // end::reference[]

#define BOOST_PLAT_WINDOWS_RUNTIME BOOST_VERSION_NUMBER_NOT_AVAILABLE

#if BOOST_OS_WINDOWS && \
    (BOOST_PLAT_WINDOWS_STORE || BOOST_PLAT_WINDOWS_PHONE)
#   undef BOOST_PLAT_WINDOWS_RUNTIME
#   define BOOST_PLAT_WINDOWS_RUNTIME BOOST_VERSION_NUMBER_AVAILABLE
#endif
 
#if BOOST_PLAT_WINDOWS_RUNTIME
#   define BOOST_PLAT_WINDOWS_RUNTIME_AVAILABLE
#   include <boost/predef/detail/platform_detected.h>
#endif

#define BOOST_PLAT_WINDOWS_RUNTIME_NAME "Windows Runtime"

#endif

#include <boost/predef/detail/test.h>
BOOST_PREDEF_DECLARE_TEST(BOOST_PLAT_WINDOWS_RUNTIME,BOOST_PLAT_WINDOWS_RUNTIME_NAME)
