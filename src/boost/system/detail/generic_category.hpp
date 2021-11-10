#ifndef BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/generic_category_message.hpp>
#include <boost/system/detail/config.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace system
{

namespace detail
{

// generic_error_category

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class BOOST_SYMBOL_VISIBLE generic_error_category: public error_category
{
public:

    BOOST_SYSTEM_CONSTEXPR generic_error_category() BOOST_NOEXCEPT:
        error_category( detail::generic_category_id )
    {
    }

    const char * name() const BOOST_NOEXCEPT BOOST_OVERRIDE
    {
        return "generic";
    }

    std::string message( int ev ) const BOOST_OVERRIDE;
    char const * message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT BOOST_OVERRIDE;
};

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic pop
#endif

// generic_error_category::message

inline char const * generic_error_category::message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT
{
    return generic_error_category_message( ev, buffer, len );
}

inline std::string generic_error_category::message( int ev ) const
{
    return generic_error_category_message( ev );
}

} // namespace detail

// generic_category()

#if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

namespace detail
{

template<class T> struct BOOST_SYMBOL_VISIBLE generic_cat_holder
{
    static constexpr generic_error_category instance{};
};

// Before C++17 it was mandatory to redeclare all static constexpr
#if defined(BOOST_NO_CXX17_INLINE_VARIABLES)
template<class T> constexpr generic_error_category generic_cat_holder<T>::instance;
#endif

} // namespace detail

constexpr error_category const & generic_category() BOOST_NOEXCEPT
{
    return detail::generic_cat_holder<void>::instance;
}

#else // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

#if !defined(__SUNPRO_CC) // trailing __global is not supported
inline error_category const & generic_category() BOOST_NOEXCEPT BOOST_SYMBOL_VISIBLE;
#endif

inline error_category const & generic_category() BOOST_NOEXCEPT
{
    static const detail::generic_error_category instance;
    return instance;
}

#endif // #if defined(BOOST_SYSTEM_HAS_CONSTEXPR)

// deprecated synonyms

#ifdef BOOST_SYSTEM_ENABLE_DEPRECATED

BOOST_SYSTEM_DEPRECATED("please use generic_category()") inline const error_category & get_generic_category() { return generic_category(); }
BOOST_SYSTEM_DEPRECATED("please use generic_category()") inline const error_category & get_posix_category() { return generic_category(); }
BOOST_SYSTEM_DEPRECATED("please use generic_category()") static const error_category & posix_category BOOST_ATTRIBUTE_UNUSED = generic_category();
BOOST_SYSTEM_DEPRECATED("please use generic_category()") static const error_category & errno_ecat BOOST_ATTRIBUTE_UNUSED = generic_category();

#endif

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_GENERIC_CATEGORY_HPP_INCLUDED
