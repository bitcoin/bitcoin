#ifndef BOOST_SYSTEM_DETAIL_ERROR_CATEGORY_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_ERROR_CATEGORY_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/config.hpp>
#include <string>
#include <functional>
#include <cstddef>

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)
# include <system_error>
# include <atomic>
#endif

namespace boost
{

namespace system
{

class error_category;
class error_code;
class error_condition;

std::size_t hash_value( error_code const & ec );

namespace detail
{

BOOST_SYSTEM_CONSTEXPR bool failed_impl( int ev, error_category const & cat );

class std_category;

} // namespace detail

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

class BOOST_SYMBOL_VISIBLE error_category
{
private:

    friend std::size_t hash_value( error_code const & ec );
    friend BOOST_SYSTEM_CONSTEXPR bool detail::failed_impl( int ev, error_category const & cat );

#if !defined(BOOST_NO_CXX11_DELETED_FUNCTIONS)
public:

    error_category( error_category const & ) = delete;
    error_category& operator=( error_category const & ) = delete;

#else
private:

    error_category( error_category const & );
    error_category& operator=( error_category const & );

#endif

private:

    boost::ulong_long_type id_;

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

    mutable std::atomic< boost::system::detail::std_category* > ps_;

#else

    boost::system::detail::std_category* ps_;

#endif

protected:

#if !defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) && !defined(BOOST_NO_CXX11_NON_PUBLIC_DEFAULTED_FUNCTIONS)

    ~error_category() = default;

#else

    // We'd like to make the destructor protected, to make code that deletes
    // an error_category* not compile; unfortunately, doing the below makes
    // the destructor user-provided and hence breaks use after main, as the
    // categories may get destroyed before code that uses them

    // ~error_category() {}

#endif

    BOOST_SYSTEM_CONSTEXPR error_category() BOOST_NOEXCEPT: id_( 0 ), ps_()
    {
    }

    explicit BOOST_SYSTEM_CONSTEXPR error_category( boost::ulong_long_type id ) BOOST_NOEXCEPT: id_( id ), ps_()
    {
    }

public:

    virtual const char * name() const BOOST_NOEXCEPT = 0;

    virtual error_condition default_error_condition( int ev ) const BOOST_NOEXCEPT;
    virtual bool equivalent( int code, const error_condition & condition ) const BOOST_NOEXCEPT;
    virtual bool equivalent( const error_code & code, int condition ) const BOOST_NOEXCEPT;

    virtual std::string message( int ev ) const = 0;
    virtual char const * message( int ev, char * buffer, std::size_t len ) const BOOST_NOEXCEPT;

    virtual bool failed( int ev ) const BOOST_NOEXCEPT
    {
        return ev != 0;
    }

    friend BOOST_SYSTEM_CONSTEXPR bool operator==( error_category const & lhs, error_category const & rhs ) BOOST_NOEXCEPT
    {
        return rhs.id_ == 0? &lhs == &rhs: lhs.id_ == rhs.id_;
    }

    friend BOOST_SYSTEM_CONSTEXPR bool operator!=( error_category const & lhs, error_category const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    friend BOOST_SYSTEM_CONSTEXPR bool operator<( error_category const & lhs, error_category const & rhs ) BOOST_NOEXCEPT
    {
        if( lhs.id_ < rhs.id_ )
        {
            return true;
        }

        if( lhs.id_ > rhs.id_ )
        {
            return false;
        }

        if( rhs.id_ != 0 )
        {
            return false; // equal
        }

        return std::less<error_category const *>()( &lhs, &rhs );
    }

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)
# if defined(__SUNPRO_CC) // trailing __global is not supported
    operator std::error_category const & () const;
# else
    operator std::error_category const & () const BOOST_SYMBOL_VISIBLE;
# endif
#endif
};

#if ( defined( BOOST_GCC ) && BOOST_GCC >= 40600 ) || defined( BOOST_CLANG )
#pragma GCC diagnostic pop
#endif

namespace detail
{

static const boost::ulong_long_type generic_category_id = ( boost::ulong_long_type( 0xB2AB117A ) << 32 ) + 0x257EDF0D;
static const boost::ulong_long_type system_category_id = ( boost::ulong_long_type( 0x8FAFD21E ) << 32 ) + 0x25C5E09B;
static const boost::ulong_long_type interop_category_id = ( boost::ulong_long_type( 0x943F2817 ) << 32 ) + 0xFD3A8FAF;

BOOST_SYSTEM_CONSTEXPR inline bool failed_impl( int ev, error_category const & cat )
{
    if( cat.id_ == system_category_id || cat.id_ == generic_category_id )
    {
        return ev != 0;
    }
    else
    {
        return cat.failed( ev );
    }
}

} // namespace detail

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_ERROR_CATEGORY_HPP_INCLUDED
