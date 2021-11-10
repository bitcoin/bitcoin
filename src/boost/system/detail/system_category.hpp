#ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/config.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace system
{

namespace detail
{

// system_error_category

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class BOOST_SYMBOL_VISIBLE system_error_category: public error_category
{
public:

    BOOST_SYSTEM_CONSTEXPR system_error_category() BOOST_NOEXCEPT:
        error_category( detail::system_category_id )
    {
    }

    const char * name() const BOOST_NOEXCEPT BOOST_OVERRIDE
    {
        return "system";
    }

    error_condition default_error_condition( int ev ) const BOOST_NOEXCEPT BOOST_OVERRIDE;

    std::string message( int ev ) const BOOST_OVERRIDE;
    char const * message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT BOOST_OVERRIDE;
};

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic pop
#endif

} // namespace detail

// system_category()

#if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

namespace detail
{

template<class T> struct BOOST_SYMBOL_VISIBLE system_cat_holder
{
    static constexpr system_error_category instance{};
};

// Before C++17 it was mandatory to redeclare all static constexpr
#if defined(BOOST_NO_CXX17_INLINE_VARIABLES)
template<class T> constexpr system_error_category system_cat_holder<T>::instance;
#endif

} // namespace detail

constexpr error_category const & system_category() BOOST_NOEXCEPT
{
    return detail::system_cat_holder<void>::instance;
}

#else // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

#if !defined(__SUNPRO_CC) // trailing __global is not supported
inline error_category const & system_category() BOOST_NOEXCEPT BOOST_SYMBOL_VISIBLE;
#endif

inline error_category const & system_category() BOOST_NOEXCEPT
{
    static const detail::system_error_category instance;
    return instance;
}

#endif // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

// deprecated synonyms

#ifdef BOOST_SYSTEM_ENABLE_DEPRECATED

BOOST_SYSTEM_DEPRECATED("please use system_category()") inline const error_category & get_system_category() { return system_category(); }
BOOST_SYSTEM_DEPRECATED("please use system_category()") static const error_category & native_ecat BOOST_ATTRIBUTE_UNUSED = system_category();

#endif

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_SYSTEM_CATEGORY_HPP_INCLUDED
