#ifndef BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_MESSAGE_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_MESSAGE_HPP_INCLUDED

// Implementation of generic_error_category_message
//
// Copyright 2018 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See library home page at http://www.boost.org/libs/system

#include <boost/config.hpp>
#include <string>
#include <cstring>

namespace boost
{

namespace system
{

namespace detail
{

#if defined(__GLIBC__)

// glibc has two incompatible strerror_r definitions

inline char const * strerror_r_helper( char const * r, char const * ) BOOST_NOEXCEPT
{
    return r;
}

inline char const * strerror_r_helper( int r, char const * buffer ) BOOST_NOEXCEPT
{
    return r == 0? buffer: "Unknown error";
}

inline char const * generic_error_category_message( int ev, char * buffer, std::size_t len ) BOOST_NOEXCEPT
{
    return strerror_r_helper( strerror_r( ev, buffer, len ), buffer );
}

inline std::string generic_error_category_message( int ev )
{
    char buffer[ 128 ];
    return generic_error_category_message( ev, buffer, sizeof( buffer ) );
}

#else // #if defined(__GLIBC__)

// std::strerror is thread-safe on everything else, incl. Windows

# if defined( BOOST_MSVC )
#  pragma warning( push )
#  pragma warning( disable: 4996 )
# elif defined(__clang__) && defined(__has_warning)
#  pragma clang diagnostic push
#  if __has_warning("-Wdeprecated-declarations")
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  endif
# endif

inline std::string generic_error_category_message( int ev )
{
    char const * m = std::strerror( ev );
    return m? m: "Unknown error";
}

inline char const * generic_error_category_message( int ev, char * buffer, std::size_t len ) BOOST_NOEXCEPT
{
    if( len == 0 )
    {
        return buffer;
    }

    if( len == 1 )
    {
        buffer[0] = 0;
        return buffer;
    }

    char const * m = std::strerror( ev );

    if( m == 0 ) return "Unknown error";

    std::strncpy( buffer, m, len - 1 );
    buffer[ len-1 ] = 0;

    return buffer;
}

# if defined( BOOST_MSVC )
#  pragma warning( pop )
# elif defined(__clang__) && defined(__has_warning)
#  pragma clang diagnostic pop
# endif

#endif // #if defined(__GLIBC__)

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_MESSAGE_HPP_INCLUDED
