#ifndef BOOST_SYSTEM_DETAIL_SNPRINTF_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_SNPRINTF_HPP_INCLUDED

// Copyright 2018, 2020, 2021 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See library home page at http://www.boost.org/libs/system

#include <boost/config.hpp>
#include <cstdio>
#include <cstdarg>

//

namespace boost
{

namespace system
{

namespace detail
{

#if ( defined(_MSC_VER) && _MSC_VER < 1900 ) || ( defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR) )

inline void snprintf( char * buffer, std::size_t len, char const * format, ... )
{
# if defined( BOOST_MSVC )
#  pragma warning( push )
#  pragma warning( disable: 4996 )
# endif

    if( len == 0 ) return;

    va_list args;
    va_start( args, format );

    _vsnprintf( buffer, len - 1, format, args );
    buffer[ len - 1 ] = 0;

    va_end( args );

# if defined( BOOST_MSVC )
#  pragma warning( pop )
# endif
}

#else

inline void snprintf( char * buffer, std::size_t len, char const * format, ... )
{
    va_list args;
    va_start( args, format );

    std::vsnprintf( buffer, len, format, args );

    va_end( args );
}

#endif

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_SNPRINTF_HPP_INCLUDED
