/*
 * Copyright 2016 Klemens D. Morgenstern
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_ENVIRONMENT_HPP_INCLUDED_
#define BOOST_WINAPI_ENVIRONMENT_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#include <boost/winapi/detail/header.hpp>

#if defined(GetEnvironmentStrings)
// Unlike most of the WinAPI, GetEnvironmentStrings is a real function and GetEnvironmentStringsA is a macro.
// In UNICODE builds, GetEnvironmentStrings is also defined as a macro that redirects to GetEnvironmentStringsW,
// and the narrow character version become inaccessible. Facepalm.
#if defined(_MSC_VER) || defined(__GNUC__)
#pragma push_macro("GetEnvironmentStrings")
#endif
#undef GetEnvironmentStrings
#define BOOST_WINAPI_DETAIL_GET_ENVIRONMENT_STRINGS_UNDEFINED
#endif // defined(GetEnvironmentStrings)

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
#if !defined( BOOST_NO_ANSI_APIS )
BOOST_WINAPI_IMPORT boost::winapi::LPSTR_ BOOST_WINAPI_WINAPI_CC GetEnvironmentStrings();
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC FreeEnvironmentStringsA(boost::winapi::LPSTR_);

BOOST_WINAPI_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC GetEnvironmentVariableA(
    boost::winapi::LPCSTR_ lpName,
    boost::winapi::LPSTR_ lpBuffer,
    boost::winapi::DWORD_ nSize
);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC SetEnvironmentVariableA(
    boost::winapi::LPCSTR_ lpName,
    boost::winapi::LPCSTR_ lpValue
);
#endif // !defined( BOOST_NO_ANSI_APIS )

BOOST_WINAPI_IMPORT boost::winapi::LPWSTR_ BOOST_WINAPI_WINAPI_CC GetEnvironmentStringsW();
BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC FreeEnvironmentStringsW(boost::winapi::LPWSTR_);

BOOST_WINAPI_IMPORT boost::winapi::DWORD_ BOOST_WINAPI_WINAPI_CC GetEnvironmentVariableW(
    boost::winapi::LPCWSTR_ lpName,
    boost::winapi::LPWSTR_ lpBuffer,
    boost::winapi::DWORD_ nSize
);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC SetEnvironmentVariableW(
    boost::winapi::LPCWSTR_ lpName,
    boost::winapi::LPCWSTR_ lpValue
);
} // extern "C"
#endif // !defined( BOOST_USE_WINDOWS_H )

namespace boost {
namespace winapi {

#if !defined( BOOST_NO_ANSI_APIS )
using ::GetEnvironmentStrings;
using ::FreeEnvironmentStringsA;
using ::GetEnvironmentVariableA;
using ::SetEnvironmentVariableA;
#endif // !defined( BOOST_NO_ANSI_APIS )

using ::GetEnvironmentStringsW;
using ::FreeEnvironmentStringsW;
using ::GetEnvironmentVariableW;
using ::SetEnvironmentVariableW;

template< typename Char >
Char* get_environment_strings();

#if !defined( BOOST_NO_ANSI_APIS )

template< >
BOOST_FORCEINLINE char* get_environment_strings< char >()
{
    return GetEnvironmentStrings();
}

BOOST_FORCEINLINE BOOL_ free_environment_strings(LPSTR_ p)
{
    return FreeEnvironmentStringsA(p);
}

BOOST_FORCEINLINE DWORD_ get_environment_variable(LPCSTR_ name, LPSTR_ buffer, DWORD_ size)
{
    return GetEnvironmentVariableA(name, buffer, size);
}

BOOST_FORCEINLINE BOOL_ set_environment_variable(LPCSTR_ name, LPCSTR_ value)
{
    return SetEnvironmentVariableA(name, value);
}

#endif // !defined( BOOST_NO_ANSI_APIS )

template< >
BOOST_FORCEINLINE wchar_t* get_environment_strings< wchar_t >()
{
    return GetEnvironmentStringsW();
}

BOOST_FORCEINLINE BOOL_ free_environment_strings(LPWSTR_ p)
{
    return FreeEnvironmentStringsW(p);
}

BOOST_FORCEINLINE DWORD_ get_environment_variable(LPCWSTR_ name, LPWSTR_ buffer, DWORD_ size)
{
    return GetEnvironmentVariableW(name, buffer, size);
}

BOOST_FORCEINLINE BOOL_ set_environment_variable(LPCWSTR_ name, LPCWSTR_ value)
{
    return SetEnvironmentVariableW(name, value);
}

} // namespace winapi
} // namespace boost

#if defined(BOOST_WINAPI_DETAIL_GET_ENVIRONMENT_STRINGS_UNDEFINED)
#if defined(_MSC_VER) || defined(__GNUC__)
#pragma pop_macro("GetEnvironmentStrings")
#elif defined(UNICODE)
#define GetEnvironmentStrings GetEnvironmentStringsW
#endif
#undef BOOST_WINAPI_DETAIL_GET_ENVIRONMENT_STRINGS_UNDEFINED
#endif // defined(BOOST_WINAPI_DETAIL_GET_ENVIRONMENT_STRINGS_UNDEFINED)

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_DESKTOP || BOOST_WINAPI_PARTITION_SYSTEM

#endif // BOOST_WINAPI_ENVIRONMENT_HPP_INCLUDED_
