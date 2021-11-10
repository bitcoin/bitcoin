/*
 * Copyright 2010 Vicente J. Botet Escriba
 * Copyright 2015 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_SEMAPHORE_HPP_INCLUDED_
#define BOOST_WINAPI_SEMAPHORE_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {

#if BOOST_WINAPI_PARTITION_APP_SYSTEM
#if !defined( BOOST_NO_ANSI_APIS )

BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateSemaphoreA(
    ::_SECURITY_ATTRIBUTES* lpSemaphoreAttributes,
    boost::winapi::LONG_ lInitialCount,
    boost::winapi::LONG_ lMaximumCount,
    boost::winapi::LPCSTR_ lpName);

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateSemaphoreExA(
    ::_SECURITY_ATTRIBUTES* lpSemaphoreAttributes,
    boost::winapi::LONG_ lInitialCount,
    boost::winapi::LONG_ lMaximumCount,
    boost::winapi::LPCSTR_ lpName,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ dwDesiredAccess);
#endif

#endif // !defined( BOOST_NO_ANSI_APIS )

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateSemaphoreW(
    ::_SECURITY_ATTRIBUTES* lpSemaphoreAttributes,
    boost::winapi::LONG_ lInitialCount,
    boost::winapi::LONG_ lMaximumCount,
    boost::winapi::LPCWSTR_ lpName);

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateSemaphoreExW(
    ::_SECURITY_ATTRIBUTES* lpSemaphoreAttributes,
    boost::winapi::LONG_ lInitialCount,
    boost::winapi::LONG_ lMaximumCount,
    boost::winapi::LPCWSTR_ lpName,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ dwDesiredAccess);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
ReleaseSemaphore(
    boost::winapi::HANDLE_ hSemaphore,
    boost::winapi::LONG_ lReleaseCount,
    boost::winapi::LPLONG_ lpPreviousCount);

#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM

#if BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
OpenSemaphoreA(
    boost::winapi::DWORD_ dwDesiredAccess,
    boost::winapi::BOOL_ bInheritHandle,
    boost::winapi::LPCSTR_ lpName);
#endif // !defined( BOOST_NO_ANSI_APIS )

BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
OpenSemaphoreW(
    boost::winapi::DWORD_ dwDesiredAccess,
    boost::winapi::BOOL_ bInheritHandle,
    boost::winapi::LPCWSTR_ lpName);

#endif // BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM

} // extern "C"
#endif // !defined( BOOST_USE_WINDOWS_H )

namespace boost {
namespace winapi {

#if BOOST_WINAPI_PARTITION_APP_SYSTEM

using ::ReleaseSemaphore;

#if defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ SEMAPHORE_ALL_ACCESS_ = SEMAPHORE_ALL_ACCESS;
BOOST_CONSTEXPR_OR_CONST DWORD_ SEMAPHORE_MODIFY_STATE_ = SEMAPHORE_MODIFY_STATE;

#else // defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ SEMAPHORE_ALL_ACCESS_ = 0x001F0003;
BOOST_CONSTEXPR_OR_CONST DWORD_ SEMAPHORE_MODIFY_STATE_ = 0x00000002;

#endif // defined( BOOST_USE_WINDOWS_H )

// Undocumented and not present in Windows SDK. Enables NtQuerySemaphore.
// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FEvent%2FNtQueryEvent.html
BOOST_CONSTEXPR_OR_CONST DWORD_ SEMAPHORE_QUERY_STATE_ = 0x00000001;

BOOST_CONSTEXPR_OR_CONST DWORD_ semaphore_all_access = SEMAPHORE_ALL_ACCESS_;
BOOST_CONSTEXPR_OR_CONST DWORD_ semaphore_modify_state = SEMAPHORE_MODIFY_STATE_;


#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ CreateSemaphoreA(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCSTR_ lpName)
{
    return ::CreateSemaphoreA(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpSemaphoreAttributes), lInitialCount, lMaximumCount, lpName);
}

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_FORCEINLINE HANDLE_ CreateSemaphoreExA(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCSTR_ lpName, DWORD_ dwFlags, DWORD_ dwDesiredAccess)
{
    return ::CreateSemaphoreExA(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpSemaphoreAttributes), lInitialCount, lMaximumCount, lpName, dwFlags, dwDesiredAccess);
}
#endif
#endif // !defined( BOOST_NO_ANSI_APIS )

BOOST_FORCEINLINE HANDLE_ CreateSemaphoreW(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCWSTR_ lpName)
{
    return ::CreateSemaphoreW(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpSemaphoreAttributes), lInitialCount, lMaximumCount, lpName);
}

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_FORCEINLINE HANDLE_ CreateSemaphoreExW(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCWSTR_ lpName, DWORD_ dwFlags, DWORD_ dwDesiredAccess)
{
    return ::CreateSemaphoreExW(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpSemaphoreAttributes), lInitialCount, lMaximumCount, lpName, dwFlags, dwDesiredAccess);
}
#endif

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ create_semaphore(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCSTR_ lpName)
{
    return winapi::CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
}
#endif

BOOST_FORCEINLINE HANDLE_ create_semaphore(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount, LPCWSTR_ lpName)
{
    return winapi::CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
}

BOOST_FORCEINLINE HANDLE_ create_anonymous_semaphore(SECURITY_ATTRIBUTES_* lpSemaphoreAttributes, LONG_ lInitialCount, LONG_ lMaximumCount)
{
    return winapi::CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, 0);
}

#endif // BOOST_WINAPI_PARTITION_APP_SYSTEM

#if BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM

#if !defined( BOOST_NO_ANSI_APIS )
using ::OpenSemaphoreA;

BOOST_FORCEINLINE HANDLE_ open_semaphore(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCSTR_ lpName)
{
    return ::OpenSemaphoreA(dwDesiredAccess, bInheritHandle, lpName);
}
#endif // !defined( BOOST_NO_ANSI_APIS )

using ::OpenSemaphoreW;

BOOST_FORCEINLINE HANDLE_ open_semaphore(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCWSTR_ lpName)
{
    return ::OpenSemaphoreW(dwDesiredAccess, bInheritHandle, lpName);
}

#endif // BOOST_WINAPI_PARTITION_DESKTOP_SYSTEM

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_SEMAPHORE_HPP_INCLUDED_
