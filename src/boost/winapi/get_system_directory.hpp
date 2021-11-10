/*
 * Copyright 2016 Klemens D. Morgenstern
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_GET_SYSTEM_DIRECTORY_HPP_INCLUDED_
#define BOOST_WINAPI_GET_SYSTEM_DIRECTORY_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_WINAPI_PARTITION_DESKTOP

#include <boost/winapi/detail/header.hpp>

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
#if !defined( BOOST_NO_ANSI_APIS )
BOOST_WINAPI_IMPORT boost::winapi::UINT_ BOOST_WINAPI_WINAPI_CC
GetSystemDirectoryA(
    boost::winapi::LPSTR_  lpBuffer,
    boost::winapi::UINT_   uSize);
#endif

BOOST_WINAPI_IMPORT boost::winapi::UINT_ BOOST_WINAPI_WINAPI_CC
GetSystemDirectoryW(
    boost::winapi::LPWSTR_ lpBuffer,
    boost::winapi::UINT_   uSize);
} // extern "C"
#endif

namespace boost {
namespace winapi {

#if !defined( BOOST_NO_ANSI_APIS )
using ::GetSystemDirectoryA;
#endif
using ::GetSystemDirectoryW;

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE UINT_ get_system_directory(LPSTR_ lpBuffer,  UINT_ uSize)
{
    return ::GetSystemDirectoryA(lpBuffer, uSize);
}
#endif

BOOST_FORCEINLINE UINT_ get_system_directory(LPWSTR_ lpBuffer,  UINT_ uSize)
{
    return ::GetSystemDirectoryW(lpBuffer, uSize);
}

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_DESKTOP

#endif // BOOST_WINAPI_GET_SYSTEM_DIRECTORY_HPP_INCLUDED_
