/*
 * Copyright 2010 Vicente J. Botet Escriba
 * Copyright 2015, 2017 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_HEAP_MEMORY_HPP_INCLUDED_
#define BOOST_WINAPI_HEAP_MEMORY_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
#undef HeapAlloc
extern "C" {

#if BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM
BOOST_WINAPI_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC
GetProcessHeaps(boost::winapi::DWORD_ NumberOfHeaps, boost::winapi::PHANDLE_ ProcessHeaps);
#endif // BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
GetProcessHeap(BOOST_WINAPI_DETAIL_VOID);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::LPVOID_ BOOST_WINAPI_WINAPI_CC
HeapAlloc(
    boost::winapi::HANDLE_ hHeap,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::SIZE_T_ dwBytes);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
HeapFree(
    boost::winapi::HANDLE_ hHeap,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::LPVOID_ lpMem);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::LPVOID_ BOOST_WINAPI_WINAPI_CC
HeapReAlloc(
    boost::winapi::HANDLE_ hHeap,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::LPVOID_ lpMem,
    boost::winapi::SIZE_T_ dwBytes);

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
HeapCreate(
    boost::winapi::DWORD_ flOptions,
    boost::winapi::SIZE_T_ dwInitialSize,
    boost::winapi::SIZE_T_ dwMaximumSize);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
HeapDestroy(boost::winapi::HANDLE_ hHeap);
#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM

} // extern "C"
#endif // !defined( BOOST_USE_WINDOWS_H )

namespace boost {
namespace winapi {

#if BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM
using ::GetProcessHeaps;
#endif

using ::GetProcessHeap;
using ::HeapAlloc;
using ::HeapFree;
using ::HeapReAlloc;

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
using ::HeapCreate;
using ::HeapDestroy;
#endif

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_HEAP_MEMORY_HPP_INCLUDED_
