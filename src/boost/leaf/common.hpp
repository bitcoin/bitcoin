#ifndef BOOST_LEAF_COMMON_HPP_INCLUDED
#define BOOST_LEAF_COMMON_HPP_INCLUDED

/// Copyright (c) 2018-2021 Emil Dotchevski and Reverge Studios, Inc.

/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LEAF_ENABLE_WARNINGS ///
#   if defined(_MSC_VER) ///
#       pragma warning(push,1) ///
#   elif defined(__clang__) ///
#       pragma clang system_header ///
#   elif (__GNUC__*100+__GNUC_MINOR__>301) ///
#       pragma GCC system_header ///
#   endif ///
#endif ///

#include <boost/leaf/detail/print.hpp>
#include <string>
#include <cerrno>
#ifdef _WIN32
#   include <Windows.h>
#   include <cstring>
#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif
#endif

namespace boost { namespace leaf {

struct BOOST_LEAF_SYMBOL_VISIBLE e_api_function { char const * value; };

struct BOOST_LEAF_SYMBOL_VISIBLE e_file_name { std::string value; };

struct BOOST_LEAF_SYMBOL_VISIBLE e_errno
{
    int value;

    explicit e_errno(int value=errno): value(value) { }

    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits> & operator<<( std::basic_ostream<CharT, Traits> & os, e_errno const & err )
    {
        return os << type<e_errno>() << ": " << err.value << ", \"" << std::strerror(err.value) << '"';
    }
};

struct BOOST_LEAF_SYMBOL_VISIBLE e_type_info_name { char const * value; };

struct BOOST_LEAF_SYMBOL_VISIBLE e_at_line { int value; };

namespace windows
{
    struct e_LastError
    {
        unsigned value;

        explicit e_LastError(unsigned value): value(value) { }

#ifdef _WIN32
        e_LastError(): value(GetLastError()) { }

        template <class CharT, class Traits>
        friend std::basic_ostream<CharT, Traits> & operator<<( std::basic_ostream<CharT, Traits> & os, e_LastError const & err )
        {
            struct msg_buf
            {
                LPVOID * p;
                msg_buf(): p(0) { }
                ~msg_buf() noexcept { if(p) LocalFree(p); }
            };
            msg_buf mb;
            if( FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
                0,
                err.value,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                (LPSTR)&mb.p,
                0,
                0) )
            {
                BOOST_LEAF_ASSERT(mb.p != 0);
                char * z = std::strchr((LPSTR)mb.p,0);
                if( z[-1] == '\n' )
                    *--z = 0;
                if( z[-1] == '\r' )
                    *--z = 0;
                return os << type<e_LastError>() << ": " << err.value << ", \"" << (LPCSTR)mb.p << '"';
            }
            return os;
        }
#else
        // TODO : Other platforms
#endif
    };
}

} }

#if defined(_MSC_VER) && !defined(BOOST_LEAF_ENABLE_WARNINGS) ///
#pragma warning(pop) ///
#endif ///

#endif
