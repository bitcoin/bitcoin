/*
 * Copyright 2020 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_GET_PROC_ADDRESS_HPP_INCLUDED_
#define BOOST_WINAPI_GET_PROC_ADDRESS_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#include <boost/winapi/detail/header.hpp>

#if !defined(BOOST_USE_WINDOWS_H)
namespace boost { namespace winapi {
#ifdef _WIN64
typedef INT_PTR_ (BOOST_WINAPI_WINAPI_CC *FARPROC_)();
typedef INT_PTR_ (BOOST_WINAPI_WINAPI_CC *NEARPROC_)();
typedef INT_PTR_ (BOOST_WINAPI_WINAPI_CC *PROC_)();
#else
typedef int (BOOST_WINAPI_WINAPI_CC *FARPROC_)();
typedef int (BOOST_WINAPI_WINAPI_CC *NEARPROC_)();
typedef int (BOOST_WINAPI_WINAPI_CC *PROC_)();
#endif // _WIN64
}} // namespace boost::winapi

extern "C" {
#if !defined(UNDER_CE)
BOOST_WINAPI_IMPORT boost::winapi::FARPROC_ BOOST_WINAPI_WINAPI_CC
GetProcAddress(boost::winapi::HMODULE_ hModule, boost::winapi::LPCSTR_ lpProcName);
#else
// On Windows CE there are two functions: GetProcAddressA (since Windows CE 3.0) and GetProcAddressW.
// GetProcAddress is a macro that is _always_ defined to GetProcAddressW.
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::FARPROC_ BOOST_WINAPI_WINAPI_CC
GetProcAddressA(boost::winapi::HMODULE_ hModule, boost::winapi::LPCSTR_ lpProcName);
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::FARPROC_ BOOST_WINAPI_WINAPI_CC
GetProcAddressW(boost::winapi::HMODULE_ hModule, boost::winapi::LPCWSTR_ lpProcName);
#endif
} // extern "C"
#endif // !defined(BOOST_USE_WINDOWS_H)

namespace boost {
namespace winapi {

#if defined(BOOST_USE_WINDOWS_H)
typedef ::FARPROC FARPROC_;
typedef ::NEARPROC NEARPROC_;
typedef ::PROC PROC_;
#endif // defined(BOOST_USE_WINDOWS_H)

#if !defined(UNDER_CE)
// For backward compatibility, don't use directly. Use get_proc_address instead.
using ::GetProcAddress;
#else
using ::GetProcAddressA;
using ::GetProcAddressW;
#endif

BOOST_FORCEINLINE FARPROC_ get_proc_address(HMODULE_ hModule, LPCSTR_ lpProcName)
{
#if !defined(UNDER_CE)
    return ::GetProcAddress(hModule, lpProcName);
#else
    return ::GetProcAddressA(hModule, lpProcName);
#endif
}

} // namespace winapi
} // namespace boost

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM
#endif // BOOST_WINAPI_GET_PROC_ADDRESS_HPP_INCLUDED_
