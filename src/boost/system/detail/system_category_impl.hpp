#ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_IMPL_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_IMPL_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018, 2020
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/system_category.hpp>
#include <boost/system/detail/error_condition.hpp>
#include <boost/system/api_config.hpp>

#if !defined(BOOST_POSIX_API) && !defined(BOOST_WINDOWS_API)
#  error BOOST_POSIX_API or BOOST_WINDOWS_API must be defined
#endif

// system_error_category implementation

#if defined(BOOST_WINDOWS_API)

#include <boost/system/detail/system_category_message_win32.hpp>
#include <boost/system/detail/system_category_condition_win32.hpp>

inline boost::system::error_condition boost::system::detail::system_error_category::default_error_condition( int ev ) const BOOST_NOEXCEPT
{
    int e2 = system_category_condition_win32( ev );

    if( e2 == -1 )
    {
        return error_condition( ev, *this );
    }
    else
    {
        return error_condition( boost::system::detail::generic_value_tag( e2 ) );
    }
}

inline std::string boost::system::detail::system_error_category::message( int ev ) const
{
    return system_category_message_win32( ev );
}

inline char const * boost::system::detail::system_error_category::message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT
{
    return system_category_message_win32( ev, buffer, len );
}

#else // #if defined(BOOST_WINDOWS_API)

#include <boost/system/detail/generic_category_message.hpp>

inline boost::system::error_condition boost::system::detail::system_error_category::default_error_condition( int ev ) const BOOST_NOEXCEPT
{
    return error_condition( boost::system::detail::generic_value_tag( ev ) );
}

inline std::string boost::system::detail::system_error_category::message( int ev ) const
{
    return generic_error_category_message( ev );
}

inline char const * boost::system::detail::system_error_category::message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT
{
    return generic_error_category_message( ev, buffer, len );
}

#endif // #if defined(BOOST_WINDOWS_API)

#endif // #ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_IMPL_HPP_INCLUDED
