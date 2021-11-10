/*
 * Copyright 2016 Andrey Semashev
 *
 * Distributed under the Boost Software License, Version 1.0.
 * See http://www.boost.org/LICENSE_1_0.txt
 */

#ifndef BOOST_WINAPI_LIMITS_HPP_INCLUDED_
#define BOOST_WINAPI_LIMITS_HPP_INCLUDED_

#include <boost/winapi/basic_types.hpp>
#include <boost/winapi/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace winapi {

#if defined( BOOST_USE_WINDOWS_H )

BOOST_CONSTEXPR_OR_CONST DWORD_ MAX_PATH_ = MAX_PATH;

#else

BOOST_CONSTEXPR_OR_CONST DWORD_ MAX_PATH_ = 260;

#endif

#if defined( BOOST_USE_WINDOWS_H ) && !defined( BOOST_WINAPI_IS_MINGW )

BOOST_CONSTEXPR_OR_CONST DWORD_ UNICODE_STRING_MAX_BYTES_ = UNICODE_STRING_MAX_BYTES;
BOOST_CONSTEXPR_OR_CONST DWORD_ UNICODE_STRING_MAX_CHARS_ = UNICODE_STRING_MAX_CHARS;

#else

BOOST_CONSTEXPR_OR_CONST DWORD_ UNICODE_STRING_MAX_BYTES_ = 65534;
BOOST_CONSTEXPR_OR_CONST DWORD_ UNICODE_STRING_MAX_CHARS_ = 32767;

#endif

BOOST_CONSTEXPR_OR_CONST DWORD_ max_path = MAX_PATH_;
BOOST_CONSTEXPR_OR_CONST DWORD_ unicode_string_max_bytes = UNICODE_STRING_MAX_BYTES_;
BOOST_CONSTEXPR_OR_CONST DWORD_ unicode_string_max_chars = UNICODE_STRING_MAX_CHARS_;

}
}

#include <boost/winapi/detail/footer.hpp>

#endif // BOOST_WINAPI_LIMITS_HPP_INCLUDED_
