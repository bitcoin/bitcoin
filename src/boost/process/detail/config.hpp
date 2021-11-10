// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016 Klemens D. Morgenstern
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/**
 * \file boost/process/config.hpp
 *
 * Defines various macros.
 */

#ifndef BOOST_PROCESS_DETAIL_CONFIG_HPP
#define BOOST_PROCESS_DETAIL_CONFIG_HPP

#include <boost/config.hpp>
#include <system_error>
#include <boost/system/api_config.hpp>

#include <boost/process/exception.hpp>

#if defined(BOOST_POSIX_API)
#include <errno.h>
#if defined(__GLIBC__)
#include <features.h>
#else
extern char **environ;
#endif
#elif defined(BOOST_WINDOWS_API)
#include <boost/winapi/get_last_error.hpp>
#else
#error "System API not supported by boost.process"
#endif

namespace boost { namespace process { namespace detail
{

#if !defined(BOOST_PROCESS_PIPE_SIZE)
#define BOOST_PROCESS_PIPE_SIZE 1024
#endif

#if defined(BOOST_POSIX_API)
namespace posix {namespace extensions {}}
namespace api = posix;

inline std::error_code get_last_error() noexcept
{
    return std::error_code(errno, std::system_category());
}

//copied from linux spec.
#if (_XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED) && !(_POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700)
#define BOOST_POSIX_HAS_VFORK 1
#endif

#if (_POSIX_C_SOURCE >= 199309L)
#define BOOST_POSIX_HAS_SIGTIMEDWAIT 1
#endif

#elif defined(BOOST_WINDOWS_API)
namespace windows {namespace extensions {}}
namespace api = windows;

inline std::error_code get_last_error() noexcept
{
    return std::error_code(::boost::winapi::GetLastError(), std::system_category());
}
#endif

inline void throw_last_error(const std::string & msg)
{
    throw process_error(get_last_error(), msg);
}

inline void throw_last_error(const char * msg)
{
    throw process_error(get_last_error(), msg);
}

inline void throw_last_error()
{
    throw process_error(get_last_error());
}

inline void throw_error(const std::error_code& ec)
{
    if (ec)
        throw process_error(ec);
}

inline void throw_error(const std::error_code& ec, const char* msg)
{
    if (ec)
        throw process_error(ec, msg);
}

template<typename Char> constexpr Char null_char();
template<> constexpr char     null_char<char>     (){return   '\0';}
template<> constexpr wchar_t  null_char<wchar_t>  (){return  L'\0';}

template<typename Char> constexpr Char equal_sign();
template<> constexpr char     equal_sign<char>    () {return  '='; }
template<> constexpr wchar_t  equal_sign<wchar_t> () {return L'='; }

template<typename Char> constexpr Char quote_sign();
template<> constexpr char     quote_sign<char>    () {return  '"'; }
template<> constexpr wchar_t  quote_sign<wchar_t> () {return L'"'; }

template<typename Char> constexpr Char space_sign();
template<> constexpr char     space_sign<char>    () {return  ' '; }
template<> constexpr wchar_t  space_sign<wchar_t> () {return L' '; }


}}}
#endif
