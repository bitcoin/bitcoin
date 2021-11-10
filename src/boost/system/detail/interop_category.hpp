#ifndef BOOST_SYSTEM_DETAIL_INTEROP_CATEGORY_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_INTEROP_CATEGORY_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018, 2021
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/snprintf.hpp>
#include <boost/system/detail/config.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace system
{

namespace detail
{

// interop_error_category, used for std::error_code

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class BOOST_SYMBOL_VISIBLE interop_error_category: public error_category
{
public:

    BOOST_SYSTEM_CONSTEXPR interop_error_category() BOOST_NOEXCEPT:
        error_category( detail::interop_category_id )
    {
    }

    const char * name() const BOOST_NOEXCEPT BOOST_OVERRIDE
    {
        return "std:unknown";
    }

    std::string message( int ev ) const BOOST_OVERRIDE;
    char const * message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT BOOST_OVERRIDE;
};

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic pop
#endif

inline char const * interop_error_category::message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT
{
    detail::snprintf( buffer, len, "Unknown interop error %d", ev );
    return buffer;
}

inline std::string interop_error_category::message( int ev ) const
{
    char buffer[ 48 ];
    return message( ev, buffer, sizeof( buffer ) );
}

// interop_category()

#if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

template<class T> struct BOOST_SYMBOL_VISIBLE interop_cat_holder
{
    static constexpr interop_error_category instance{};
};

// Before C++17 it was mandatory to redeclare all static constexpr
#if defined(BOOST_NO_CXX17_INLINE_VARIABLES)
template<class T> constexpr interop_error_category interop_cat_holder<T>::instance;
#endif

constexpr error_category const & interop_category() BOOST_NOEXCEPT
{
    return interop_cat_holder<void>::instance;
}

#else // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

#if !defined(__SUNPRO_CC) // trailing __global is not supported
inline error_category const & interop_category() BOOST_NOEXCEPT BOOST_SYMBOL_VISIBLE;
#endif

inline error_category const & interop_category() BOOST_NOEXCEPT
{
    static const detail::interop_error_category instance;
    return instance;
}

#endif // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_INTEROP_CATEGORY_HPP_INCLUDED
