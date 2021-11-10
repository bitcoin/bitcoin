#ifndef BOOST_SYSTEM_DETAIL_ERROR_CODE_HPP_INCLUDED
#define BOOST_SYSTEM_DETAIL_ERROR_CODE_HPP_INCLUDED

//  Copyright Beman Dawes 2006, 2007
//  Copyright Christoper Kohlhoff 2007
//  Copyright Peter Dimov 2017, 2018
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/system

#include <boost/system/is_error_code_enum.hpp>
#include <boost/system/detail/error_category.hpp>
#include <boost/system/detail/error_condition.hpp>
#include <boost/system/detail/system_category.hpp>
#include <boost/system/detail/system_category_impl.hpp>
#include <boost/system/detail/interop_category.hpp>
#include <boost/system/detail/enable_if.hpp>
#include <boost/system/detail/is_same.hpp>
#include <boost/system/detail/snprintf.hpp>
#include <boost/system/detail/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/config.hpp>
#include <ostream>
#include <new>

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)
# include <system_error>
#endif

#if defined(BOOST_GCC) && BOOST_GCC >= 40600 && BOOST_GCC < 70000
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

namespace boost
{

namespace system
{

//  class error_code

//  We want error_code to be a value type that can be copied without slicing
//  and without requiring heap allocation, but we also want it to have
//  polymorphic behavior based on the error category. This is achieved by
//  abstract base class error_category supplying the polymorphic behavior,
//  and error_code containing a pointer to an object of a type derived
//  from error_category.

std::size_t hash_value( error_code const & ec );

class error_code
{
private:

    friend std::size_t hash_value( error_code const & ec );

private:

    struct data
    {
        int val_;
        const error_category * cat_;
    };

    union
    {
        data d1_;
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)
        unsigned char d2_[ sizeof(std::error_code) ];
#endif
    };

    // 0: default constructed, d1_ value initialized
    // 1: holds std::error_code in d2_
    // 2: holds error code in d1_, failed == false
    // 3: holds error code in d1_, failed == true
    unsigned flags_;

public:

    // constructors:

    BOOST_SYSTEM_CONSTEXPR error_code() BOOST_NOEXCEPT:
        d1_(), flags_( 0 )
    {
    }

    BOOST_SYSTEM_CONSTEXPR error_code( int val, const error_category & cat ) BOOST_NOEXCEPT:
        d1_(), flags_( 2 + detail::failed_impl( val, cat ) )
    {
        d1_.val_ = val;
        d1_.cat_ = &cat;
    }

    template<class ErrorCodeEnum> BOOST_SYSTEM_CONSTEXPR error_code( ErrorCodeEnum e,
        typename detail::enable_if<is_error_code_enum<ErrorCodeEnum>::value>::type* = 0 ) BOOST_NOEXCEPT
    {
        *this = make_error_code( e );
    }

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

    error_code( std::error_code const& ec ) BOOST_NOEXCEPT:
        flags_( 1 )
    {
        ::new( d2_ ) std::error_code( ec );
    }

#endif

    // modifiers:

    BOOST_SYSTEM_CONSTEXPR void assign( int val, const error_category & cat ) BOOST_NOEXCEPT
    {
        *this = error_code( val, cat );
    }

    template<typename ErrorCodeEnum>
        BOOST_SYSTEM_CONSTEXPR typename detail::enable_if<is_error_code_enum<ErrorCodeEnum>::value, error_code>::type &
        operator=( ErrorCodeEnum val ) BOOST_NOEXCEPT
    {
        *this = make_error_code( val );
        return *this;
    }

    BOOST_SYSTEM_CONSTEXPR void clear() BOOST_NOEXCEPT
    {
        *this = error_code();
    }

    // observers:

    BOOST_SYSTEM_CONSTEXPR int value() const BOOST_NOEXCEPT
    {
        if( flags_ != 1 )
        {
            return d1_.val_;
        }
        else
        {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

            std::error_code const& ec = *reinterpret_cast<std::error_code const*>( d2_ );
            return ec.value() + 1000 * static_cast<unsigned>( reinterpret_cast<boost::uintptr_t>( &ec.category() ) % 2097143 ); // 2^21-9, prime
#else

            return -1;
#endif
        }
    }

    BOOST_SYSTEM_CONSTEXPR const error_category & category() const BOOST_NOEXCEPT
    {
        if( flags_ == 0 )
        {
            return system_category();
        }
        else if( flags_ == 1 )
        {
            return detail::interop_category();
        }
        else
        {
            return *d1_.cat_;
        }
    }

    // deprecated?
    error_condition default_error_condition() const BOOST_NOEXCEPT
    {
        return category().default_error_condition( value() );
    }

    std::string message() const
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( flags_ == 1 )
        {
            std::error_code const& ec = *reinterpret_cast<std::error_code const*>( d2_ );
            return ec.message();
        }

#endif

        return category().message( value() );
    }

    char const * message( char * buffer, std::size_t len ) const BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)
        if( flags_ == 1 )
        {
#if !defined(BOOST_NO_EXCEPTIONS)
            try
#endif
            {
                std::error_code const& ec = *reinterpret_cast<std::error_code const*>( d2_ );
                detail::snprintf( buffer, len, "%s", ec.message().c_str() );
                return buffer;
            }
#if !defined(BOOST_NO_EXCEPTIONS)
            catch( ... )
            {
            }
#endif
        }
#endif

        return category().message( value(), buffer, len );
    }

    BOOST_SYSTEM_CONSTEXPR bool failed() const BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( flags_ == 1 )
        {
            std::error_code const& ec = *reinterpret_cast<std::error_code const*>( d2_ );
            return ec.value() != 0;
        }
        else

#endif
        {
            return (flags_ & 1) != 0;
        }
    }

#if !defined(BOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)

    BOOST_SYSTEM_CONSTEXPR explicit operator bool() const BOOST_NOEXCEPT  // true if error
    {
        return failed();
    }

#else

    typedef void (*unspecified_bool_type)();
    static void unspecified_bool_true() {}

    BOOST_SYSTEM_CONSTEXPR operator unspecified_bool_type() const  BOOST_NOEXCEPT // true if error
    {
        return failed()? unspecified_bool_true: 0;
    }

    BOOST_SYSTEM_CONSTEXPR bool operator!() const BOOST_NOEXCEPT // true if no error
    {
        return !failed();
    }

#endif

    // relationals:

    //  the more symmetrical non-member syntax allows enum
    //  conversions work for both rhs and lhs.

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator==( const error_code & lhs, const error_code & rhs ) BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( lhs.flags_ == 1 && rhs.flags_ == 1 )
        {
            std::error_code const& e1 = *reinterpret_cast<std::error_code const*>( lhs.d2_ );
            std::error_code const& e2 = *reinterpret_cast<std::error_code const*>( rhs.d2_ );

            return e1 == e2;
        }
        else
#endif
        {
            return lhs.value() == rhs.value() && lhs.category() == rhs.category();
        }
    }

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator<( const error_code & lhs, const error_code & rhs ) BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( lhs.flags_ == 1 && rhs.flags_ == 1 )
        {
            std::error_code const& e1 = *reinterpret_cast<std::error_code const*>( lhs.d2_ );
            std::error_code const& e2 = *reinterpret_cast<std::error_code const*>( rhs.d2_ );

            return e1 < e2;
        }
        else
#endif
        {
            return lhs.category() < rhs.category() || (lhs.category() == rhs.category() && lhs.value() < rhs.value());
        }
    }

    BOOST_SYSTEM_CONSTEXPR inline friend bool operator!=( const error_code & lhs, const error_code & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    inline friend bool operator==( const error_code & code, const error_condition & condition ) BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( code.flags_ == 1 )
        {
            return static_cast<std::error_code>( code ) == static_cast<std::error_condition>( condition );
        }
        else

#endif
        {
            return code.category().equivalent( code.value(), condition ) || condition.category().equivalent( code, condition.value() );
        }
    }

    inline friend bool operator==( const error_condition & condition, const error_code & code ) BOOST_NOEXCEPT
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( code.flags_ == 1 )
        {
            return static_cast<std::error_code>( code ) == static_cast<std::error_condition>( condition );
        }
        else

#endif
        {
            return code.category().equivalent( code.value(), condition ) || condition.category().equivalent( code, condition.value() );
        }
    }

    inline friend bool operator!=( const error_code & lhs, const error_condition & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    inline friend bool operator!=( const error_condition & lhs, const error_code & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

    inline friend bool operator==( std::error_code const & lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return lhs == static_cast< std::error_code >( rhs );
    }

    inline friend bool operator==( error_code const & lhs, std::error_code const & rhs ) BOOST_NOEXCEPT
    {
        return static_cast< std::error_code >( lhs ) == rhs;
    }

    inline friend bool operator!=( std::error_code const & lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    inline friend bool operator!=( error_code const & lhs, std::error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    //

    template<class E, class N = typename detail::enable_if<std::is_error_condition_enum<E>::value>::type>
    inline friend bool operator==( error_code const & lhs, E rhs ) BOOST_NOEXCEPT
    {
        return lhs == make_error_condition( rhs );
    }

    template<class E, class N = typename detail::enable_if<std::is_error_condition_enum<E>::value>::type>
    inline friend bool operator==( E lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return make_error_condition( lhs ) == rhs;
    }

    template<class E, class N = typename detail::enable_if<std::is_error_condition_enum<E>::value>::type>
    inline friend bool operator!=( error_code const & lhs, E rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    template<class E, class N = typename detail::enable_if<std::is_error_condition_enum<E>::value>::type>
    inline friend bool operator!=( E lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    //

    template<class E, class N1 = void, class N2 = typename detail::enable_if<std::is_error_code_enum<E>::value>::type>
    BOOST_SYSTEM_CONSTEXPR inline friend bool operator==( error_code const & lhs, E rhs ) BOOST_NOEXCEPT
    {
        return lhs == make_error_code( rhs );
    }

    template<class E, class N1 = void, class N2 = typename detail::enable_if<std::is_error_code_enum<E>::value>::type>
    BOOST_SYSTEM_CONSTEXPR inline friend bool operator==( E lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return make_error_code( lhs ) == rhs;
    }

    template<class E, class N1 = void, class N2 = typename detail::enable_if<std::is_error_code_enum<E>::value>::type>
    BOOST_SYSTEM_CONSTEXPR inline friend bool operator!=( error_code const & lhs, E rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    template<class E, class N1 = void, class N2 = typename detail::enable_if<std::is_error_code_enum<E>::value>::type>
    BOOST_SYSTEM_CONSTEXPR inline friend bool operator!=( E lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

#if defined(BOOST_SYSTEM_CLANG_6)

    inline friend bool operator==( error_code const & lhs, std::error_condition const & rhs ) BOOST_NOEXCEPT
    {
        return static_cast< std::error_code >( lhs ) == rhs;
    }

    inline friend bool operator==( std::error_condition const & lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return lhs == static_cast< std::error_code >( rhs );
    }

    inline friend bool operator!=( error_code const & lhs, std::error_condition const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

    inline friend bool operator!=( std::error_condition const & lhs, error_code const & rhs ) BOOST_NOEXCEPT
    {
        return !( lhs == rhs );
    }

#endif

    // conversions

    operator std::error_code () const
    {
        if( flags_ == 1 )
        {
            return *reinterpret_cast<std::error_code const*>( d2_ );
        }
        else if( flags_ == 0 )
        {
            //return std::error_code();
            return std::error_code( 0, boost::system::system_category() );
        }
        else
        {
            return std::error_code( d1_.val_, *d1_.cat_ );
        }
    }

    operator std::error_code ()
    {
        return const_cast<error_code const&>( *this );
    }

    template<class T,
      class E = typename detail::enable_if<detail::is_same<T, std::error_code>::value>::type>
      operator T& ()
    {
        if( flags_ != 1 )
        {
            std::error_code e2( *this );
            ::new( d2_ ) std::error_code( e2 );
            flags_ = 1;
        }

        return *reinterpret_cast<std::error_code*>( d2_ );
    }

#if defined(BOOST_SYSTEM_CLANG_6)

    template<class T,
      class E = typename std::enable_if<std::is_same<T, std::error_code>::value>::type>
      operator T const& () = delete;

#endif

#endif

    template<class Ch, class Tr>
        inline friend std::basic_ostream<Ch, Tr>&
        operator<< (std::basic_ostream<Ch, Tr>& os, error_code const & ec)
    {
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

        if( ec.flags_ == 1 )
        {
            std::error_code const& e2 = *reinterpret_cast<std::error_code const*>( ec.d2_ );
            os << "std:" << e2.category().name() << ':' << e2.value();
        }
        else
#endif
        {
            os << ec.category().name() << ':' << ec.value();
        }

        return os;
    }
};

inline std::size_t hash_value( error_code const & ec )
{
#if defined(BOOST_SYSTEM_HAS_SYSTEM_ERROR)

    if( ec.flags_ == 1 )
    {
        std::error_code const& e2 = *reinterpret_cast<std::error_code const*>( ec.d2_ );
        return std::hash<std::error_code>()( e2 );
    }

#endif

    error_category const & cat = ec.category();

    boost::ulong_long_type id_ = cat.id_;

    if( id_ == 0 )
    {
        id_ = reinterpret_cast<boost::uintptr_t>( &cat );
    }

    boost::ulong_long_type hv = ( boost::ulong_long_type( 0xCBF29CE4 ) << 32 ) + 0x84222325;
    boost::ulong_long_type const prime = ( boost::ulong_long_type( 0x00000100 ) << 32 ) + 0x000001B3;

    // id

    hv ^= id_;
    hv *= prime;

    // value

    hv ^= static_cast<unsigned>( ec.value() );
    hv *= prime;

    return static_cast<std::size_t>( hv );
}

} // namespace system

} // namespace boost

#if defined(BOOST_GCC) && BOOST_GCC >= 40600 && BOOST_GCC < 70000
# pragma GCC diagnostic pop
#endif

#endif // #ifndef BOOST_SYSTEM_DETAIL_ERROR_CODE_HPP_INCLUDED
