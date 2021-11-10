/*
 * Copyright 2010 Vicente J. Botet Escriba
 * Copyright 2015, 2017 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_EVENT_HPP_INCLUDED_
#define BOOST_WINAPI_EVENT_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H ) && BOOST_WINAPI_PARTITION_APP_SYSTEM
extern "C" {
#if !defined( BOOST_NO_ANSI_APIS )
BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateEventA(
    ::_SECURITY_ATTRIBUTES* lpEventAttributes,
    boost::winapi::BOOL_ bManualReset,
    boost::winapi::BOOL_ bInitialState,
    boost::winapi::LPCSTR_ lpName);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateEventW(
    ::_SECURITY_ATTRIBUTES* lpEventAttributes,
    boost::winapi::BOOL_ bManualReset,
    boost::winapi::BOOL_ bInitialState,
    boost::winapi::LPCWSTR_ lpName);
} // extern "C"
#endif // !defined( BOOST_USE_WINDOWS_H ) && BOOST_WINAPI_PARTITION_APP_SYSTEM

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
#if !defined( BOOST_NO_ANSI_APIS )
#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateEventExA(
    ::_SECURITY_ATTRIBUTES *lpEventAttributes,
    boost::winapi::LPCSTR_ lpName,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ dwDesiredAccess);
#endif

BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
OpenEventA(
    boost::winapi::DWORD_ dwDesiredAccess,
    boost::winapi::BOOL_ bInheritHandle,
    boost::winapi::LPCSTR_ lpName);
#endif // !defined( BOOST_NO_ANSI_APIS )

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_WINAPI_IMPORT boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
CreateEventExW(
    ::_SECURITY_ATTRIBUTES *lpEventAttributes,
    boost::winapi::LPCWSTR_ lpName,
    boost::winapi::DWORD_ dwFlags,
    boost::winapi::DWORD_ dwDesiredAccess);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::HANDLE_ BOOST_WINAPI_WINAPI_CC
OpenEventW(
    boost::winapi::DWORD_ dwDesiredAccess,
    boost::winapi::BOOL_ bInheritHandle,
    boost::winapi::LPCWSTR_ lpName);

// Windows CE define SetEvent/ResetEvent as inline functions in kfuncs.h
#if !defined( UNDER_CE )
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
SetEvent(boost::winapi::HANDLE_ hEvent);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
ResetEvent(boost::winapi::HANDLE_ hEvent);
#endif
} // extern "C"
#endif

namespace boost {
namespace winapi {

#if !defined( BOOST_NO_ANSI_APIS )
using ::OpenEventA;
#endif
using ::OpenEventW;
using ::SetEvent;
using ::ResetEvent;

#if defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ EVENT_ALL_ACCESS_ = EVENT_ALL_ACCESS;
BOOST_CONSTEXPR_OR_CONST DWORD_ EVENT_MODIFY_STATE_ = EVENT_MODIFY_STATE;
#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_CONSTEXPR_OR_CONST DWORD_ CREATE_EVENT_INITIAL_SET_ = CREATE_EVENT_INITIAL_SET;
BOOST_CONSTEXPR_OR_CONST DWORD_ CREATE_EVENT_MANUAL_RESET_ = CREATE_EVENT_MANUAL_RESET;
#endif

#else // defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ EVENT_ALL_ACCESS_ = 0x001F0003;
BOOST_CONSTEXPR_OR_CONST DWORD_ EVENT_MODIFY_STATE_ = 0x00000002;
#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_CONSTEXPR_OR_CONST DWORD_ CREATE_EVENT_INITIAL_SET_ = 0x00000002;
BOOST_CONSTEXPR_OR_CONST DWORD_ CREATE_EVENT_MANUAL_RESET_ = 0x00000001;
#endif

#endif // defined( BOOST_USE_WINDOWS_H )

// Undocumented and not present in Windows SDK. Enables NtQueryEvent.
// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FNT%20Objects%2FEvent%2FNtQueryEvent.html
BOOST_CONSTEXPR_OR_CONST DWORD_ EVENT_QUERY_STATE_ = 0x00000001;

BOOST_CONSTEXPR_OR_CONST DWORD_ event_all_access = EVENT_ALL_ACCESS_;
BOOST_CONSTEXPR_OR_CONST DWORD_ event_modify_state = EVENT_MODIFY_STATE_;
#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_CONSTEXPR_OR_CONST DWORD_ create_event_initial_set = CREATE_EVENT_INITIAL_SET_;
BOOST_CONSTEXPR_OR_CONST DWORD_ create_event_manual_reset = CREATE_EVENT_MANUAL_RESET_;
#endif

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ CreateEventA(SECURITY_ATTRIBUTES_* lpEventAttributes, BOOL_ bManualReset, BOOL_ bInitialState, LPCSTR_ lpName)
{
#if !BOOST_WINAPI_PARTITION_APP_SYSTEM && BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    const DWORD_ flags = (bManualReset ? create_event_manual_reset : 0u) | (bInitialState ? create_event_initial_set : 0u);
    return ::CreateEventExA(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), lpName, flags, event_all_access);
#else
    return ::CreateEventA(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), bManualReset, bInitialState, lpName);
#endif
}

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_FORCEINLINE HANDLE_ CreateEventExA(SECURITY_ATTRIBUTES_* lpEventAttributes, LPCSTR_ lpName, DWORD_ dwFlags, DWORD_ dwDesiredAccess)
{
    return ::CreateEventExA(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), lpName, dwFlags, dwDesiredAccess);
}
#endif
#endif

BOOST_FORCEINLINE HANDLE_ CreateEventW(SECURITY_ATTRIBUTES_* lpEventAttributes, BOOL_ bManualReset, BOOL_ bInitialState, LPCWSTR_ lpName)
{
#if !BOOST_WINAPI_PARTITION_APP_SYSTEM && BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
    const DWORD_ flags = (bManualReset ? create_event_manual_reset : 0u) | (bInitialState ? create_event_initial_set : 0u);
    return ::CreateEventExW(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), lpName, flags, event_all_access);
#else
    return ::CreateEventW(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), bManualReset, bInitialState, lpName);
#endif
}

#if BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_WIN6
BOOST_FORCEINLINE HANDLE_ CreateEventExW(SECURITY_ATTRIBUTES_* lpEventAttributes, LPCWSTR_ lpName, DWORD_ dwFlags, DWORD_ dwDesiredAccess)
{
    return ::CreateEventExW(reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpEventAttributes), lpName, dwFlags, dwDesiredAccess);
}
#endif

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ create_event(SECURITY_ATTRIBUTES_* lpEventAttributes, BOOL_ bManualReset, BOOL_ bInitialState, LPCSTR_ lpName)
{
    return winapi::CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
}

BOOST_FORCEINLINE HANDLE_ open_event(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCSTR_ lpName)
{
    return ::OpenEventA(dwDesiredAccess, bInheritHandle, lpName);
}
#endif

BOOST_FORCEINLINE HANDLE_ create_event(SECURITY_ATTRIBUTES_* lpEventAttributes, BOOL_ bManualReset, BOOL_ bInitialState, LPCWSTR_ lpName)
{
    return winapi::CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
}

BOOST_FORCEINLINE HANDLE_ open_event(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCWSTR_ lpName)
{
    return ::OpenEventW(dwDesiredAccess, bInheritHandle, lpName);
}

BOOST_FORCEINLINE HANDLE_ create_anonymous_event(SECURITY_ATTRIBUTES_* lpEventAttributes, BOOL_ bManualReset, BOOL_ bInitialState)
{
    return winapi::CreateEventW(lpEventAttributes, bManualReset, bInitialState, 0);
}

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_EVENT_HPP_INCLUDED_
