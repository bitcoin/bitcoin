/*
 * Copyright 2016 Klemens D. Morgenstern
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_HANDLE_INFO_HPP_INCLUDED_
#define BOOST_WINAPI_HANDLE_INFO_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if BOOST_WINAPI_PARTITION_DESKTOP

#include <boost/winapi/detail/header.hpp>

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
GetHandleInformation(
    boost::winapi::HANDLE_ hObject,
    boost::winapi::LPDWORD_ lpdwFlags);

BOOST_WINAPI_IMPORT boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
SetHandleInformation(
    boost::winapi::HANDLE_ hObject,
    boost::winapi::DWORD_ dwMask,
    boost::winapi::DWORD_ dwFlags);

} // extern "C"
#endif

namespace boost {
namespace winapi {

using ::GetHandleInformation;
using ::SetHandleInformation;

#if defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ HANDLE_FLAG_INHERIT_            = HANDLE_FLAG_INHERIT;
BOOST_CONSTEXPR_OR_CONST DWORD_ HANDLE_FLAG_PROTECT_FROM_CLOSE_ = HANDLE_FLAG_PROTECT_FROM_CLOSE;

#else

BOOST_CONSTEXPR_OR_CONST DWORD_ HANDLE_FLAG_INHERIT_            = 0x1;
BOOST_CONSTEXPR_OR_CONST DWORD_ HANDLE_FLAG_PROTECT_FROM_CLOSE_ = 0x2;

#endif

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_PARTITION_DESKTOP

#endif // BOOST_WINAPI_HANDLE_INFO_HPP_INCLUDED_
