/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   snprintf.hpp
 * \author Andrey Semashev
 * \date   20.02.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_DETAIL_SNPRINTF_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SNPRINTF_HPP_INCLUDED_

#include <stdio.h>
#include <cstddef>
#include <cstdarg>
#include <boost/log/detail/config.hpp>
#ifdef BOOST_LOG_USE_WCHAR_T
#include <wchar.h>
#endif // BOOST_LOG_USE_WCHAR_T
#include <boost/log/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

#if defined(_MSC_VER) || (defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR))

// MSVC snprintfs are not conforming but they are good enough for our cases.
// MinGW32, at least the older versions up until gcc 4.7, also provide the non-conforming interface.
inline int vsnprintf(char* buf, std::size_t size, const char* format, std::va_list args)
{
    int n = _vsnprintf(buf, size, format, args);
    if (static_cast< unsigned int >(n) >= size)
    {
        n = static_cast< int >(size);
        buf[size - 1] = '\0';
    }
    return n;
}

#   ifdef BOOST_LOG_USE_WCHAR_T
inline int vswprintf(wchar_t* buf, std::size_t size, const wchar_t* format, std::va_list args)
{
    int n = _vsnwprintf(buf, size, format, args);
    if (static_cast< unsigned int >(n) >= size)
    {
        n = static_cast< int >(size);
        buf[size - 1] = L'\0';
    }
    return n;
}
#   endif // BOOST_LOG_USE_WCHAR_T

inline int snprintf(char* buf, std::size_t size, const char* format, ...)
{
    std::va_list args;
    va_start(args, format);
    int n = vsnprintf(buf, size, format, args);
    va_end(args);
    return n;
}

#   ifdef BOOST_LOG_USE_WCHAR_T
inline int swprintf(wchar_t* buf, std::size_t size, const wchar_t* format, ...)
{
    std::va_list args;
    va_start(args, format);
    int n = vswprintf(buf, size, format, args);
    va_end(args);
    return n;
}
#   endif // BOOST_LOG_USE_WCHAR_T

#else

// Standard-conforming compilers already have the correct snprintfs
using ::snprintf;
using ::vsnprintf;

#   ifdef BOOST_LOG_USE_WCHAR_T
using ::swprintf;
using ::vswprintf;
#   endif // BOOST_LOG_USE_WCHAR_T

#endif

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_DETAIL_SNPRINTF_HPP_INCLUDED_
