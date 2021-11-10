#ifndef BOOST_SYSTEM_DETAIL_ERROR_CONDITION_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_ERROR_CONDITION_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/generic_category.hpp>
#include <boost/system/detail/enable_if.hpp>
#include <boost/system/detail/is_same.hpp>
#include <boost/system/detail/errc.hpp>
#include <boost/system/is_error_condition_enum.hpp>
#include <boost/system/detail/config.hpp>
#include <boost/config.hpp>

namespace boost
{

namespace system
{

// class error_condition

// error_conditions are portable, error_codes are system or library specific

namespace detail
{

struct generic_value_tag
{
    int value;
    BOOST_SYSTEM_CONSTEXPR explicit generic_value_tag( int v ): value( v ) {}
};

} // namespace detail

class error_condition
{
private:

    int val_;
    bool failed_;
    error_category const * cat_;

public:

    // constructors:

    BOOST_SYSTEM_CONSTEXPR error_condition() BOOST_NOEXCEPT:
        val_( 0 ), failed_( false ), cat_( 0 )
    {
    }

    BOOST_SYSTEM_CONSTEXPR error_condition( int val, const error_category & cat ) BOOST_NOEXCEPT:
        val_( val ), failed_( detail::failed_impl( val, cat ) ), cat_( &cat )
    {
    }

    BOOST_SYSTEM_CONSTEXPR explicit error_condition( boost::system::detail::generic_value_tag vt ) BOOST_NOEXCEPT:
        val_( vt.value ), failed_( vt.value != 0 ), cat_( 0 )
    {
    }

    template<class ErrorConditionEnum> BOOST_SYSTEM_CONSTEXPR error_condition( ErrorConditionEnum e,
      typename detail::enable_if<
        is_error_condition_enum<ErrorConditionEnum>::value && !boost::system::detail::is_same<ErrorConditionEnum, errc::errc_t>::value
      >::type* = 0) BOOST_NOEXCEPT
    {
        *this = make_error_condition( e );
    }

    template<class ErrorConditionEnum> BOOST_SYSTEM_CONSTEXPR error_condition( ErrorConditionEnum e,
      typename detail::enable_if<boost::system::detail::is_same<ErrorConditionEnum, errc::errc_t>::value>::type* = 0) BOOST_NOEXCEPT:
        val_( e ), failed_( e != 0 ), cat_( 0 )
    {
    }

    // modifiers:

    BOOST_SYSTEM_CONSTEXPR void assign( int val, const error_category & cat ) BOOST_NOEXCEPT
    {
        val_ = val;
        failed_ = detail::failed_impl( val, cat );
        cat_ = &cat;
    }

    template<typename ErrorConditionEnum>
        BOOST_SYSTEM_CONSTEXPR typename detail::enable_if<is_error_condition_enum<ErrorConditionEnum>::value, error_condition>::type &
        operator=( ErrorConditionEnum val ) BOOST_NOEXCEPT
    {
        *this = error_condition( val );
        return *this;
    }

    BOOST_SYSTEM_CONSTEXPR void clear() BOOST_NOEXCEPT
    {
        val_ = 0;
        failed_ = false;
        cat_ = 0;
    }

    // observers:

    BOOST_SYSTEM_CONSTEXPR int value() const BOOST_NOEXCEPT
    {
        return val_;
    }

    BOOST_SYSTEM_CONSTEXPR const error_category & category() const BOOST_NOEXCEPT
    {
        return cat_? *cat_: generic_category();
    }

    std::string message() const
    {
        if( cat_ )
        {
            return cat_->message( value() );
        }
        else
        {
            return generic_category().message( value() );
        }
    }

    char const * message( char * buffer, std::size_t len ) const BOOST_NOEXCEPT
    {
        if( cat_ )
        {
            return cat_->message( value(), buffer, len );
        }
        else
        {
            return generic_category().message( value(), buffer, len );
        }
    }

    BOOST_SYSTEM_CONSTEXPR bool failed() const BOOST_NOEXCEPT
    {
        return failed_;
    }

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)

    BOOST_SYSTEM_CONSTEXPR explicit operator bool() const BOOST_NOEXCEPT  // true if error
    {
        return failed_;
    }

#else

    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {}

    BOOST_SYSTEM_CONSTEXPR operator unspecified_bool_type() const BOOST_NOEXCEPT  // true if error
    {
        return failed_? unspecified_bool_true: 0;
    }

    BOOST_SYSTEM_CONSTEXPR bool operator!() const BOOST_NOEXCEPT  // true if no error
    {
        return !failed_;
    }

#endif

    // relationals:
    //  the more symmetrical non-member syntax allows enum
    //  conversions work for both rhs and lhs.

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator==( const error_condition & lhs, const error_condition & rhs ) BOOST_NOEXCEPT
    {
        return lhs.val_ == rhs.val_ && lhs.category() == rhs.category();
    }

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator<( const error_condition & lhs, const error_condition & rhs ) BOOST_NOEXCEPT
    {
        error_category const& lcat = lhs.category();
        error_category const& rcat = rhs.category();
        return lcat < rcat || ( lcat == rcat && lhs.val_ < rhs.val_ );
    }

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator!=( const error_condition & lhs, const error_condition & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

    operator std::error_condition () const
    {
        return std::error_condition( value(), category() );
    }

    inline friend bool operator==( std::error_code const & lhs, error_condition const & rhs ) BOOST_NOEXCEPT
    {
        return lhs == static_cast< std::error_condition >( rhs );
    }

    inline friend bool operator==( error_condition const & lhs, std::error_code const & rhs ) BOOST_NOEXCEPT
    {
        return static_cast< std::error_condition >( lhs ) == rhs;
    }

    inline friend bool operator!=( std::error_code const & lhs, error_condition const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    inline friend bool operator!=( error_condition const & lhs, std::error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

#endif

    template<class Ch, class Tr>
        inline friend std::basic_ostream<Ch, Tr>&
        operator<< (std::basic_ostream<Ch, Tr>& os, error_condition const & en)
    {
        {
            os << "cond:" << en.category().name() << ':' << en.value();
        }

        return os;
    }
};

} // namespace system

} // namespace boost

#endif // #ifndef BOOST_SYSTEM_DETAIL_ERROR_CONDITION_HPP_INCLUDED
