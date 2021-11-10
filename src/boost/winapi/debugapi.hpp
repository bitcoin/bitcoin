/*
 * Copyright 2017 Vinnie Falco
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_DEBUGAPI_HPP_INCLUDED_
#define BOOST_WINAPI_DEBUGAPI_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/config.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {

#if (BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4)
BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::BOOL_ BOOST_WINAPI_WINAPI_CC
IsDebuggerPresent(BOOST_WINAPI_DETAIL_VOID);
#endif

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
OutputDebugStringA(boost::winapi::LPCSTR_);

BOOST_WINAPI_IMPORT_EXCEPT_WM boost::winapi::VOID_ BOOST_WINAPI_WINAPI_CC
OutputDebugStringW(boost::winapi::LPCWSTR_);

} // extern "C"
#endif

namespace boost {
namespace winapi {

#if (BOOST_USE_WINAPI_VERSION >= BOOST_WINAPI_VERSION_NT4)
using ::IsDebuggerPresent;
#endif

using ::OutputDebugStringA;
using ::OutputDebugStringW;

BOOST_FORCEINLINE void output_debug_string(char const* s)
{
    ::OutputDebugStringA(s);
}

BOOST_FORCEINLINE void output_debug_string(wchar_t const* s)
{
    ::OutputDebugStringW(s);
}

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_DEBUGAPI_HPP_INCLUDED_
