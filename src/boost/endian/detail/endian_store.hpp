#ifndef BOOST_ENDIAN_DETAIL_ENDIAN_STORE_HPP_INCLUDED
#define BOOST_ENDIAN_DETAIL_ENDIAN_STORE_HPP_INCLUDED

// Copyright 2019 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt

#include <boost/endian/detail/endian_reverse.hpp>
#include <boost/endian/detail/order.hpp>
#include <boost/endian/detail/integral_by_size.hpp>
#include <boost/endian/detail/is_trivially_copyable.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/static_assert.hpp>
#include <cstddef>
#include <cstring>

namespace boost
{
namespace endian
{

namespace detail
{

template<class T, std::size_t N1, BOOST_SCOPED_ENUM(order) O1, std::size_t N2, BOOST_SCOPED_ENUM(order) O2> struct endian_store_impl
{
};

} // namespace detail

// Requires:
//
//    sizeof(T) must be 1, 2, 4, or 8
//    1 <= N <= sizeof(T)
//    T is TriviallyCopyable
//    if N < sizeof(T), T is integral or enum

template<class T, std::size_t N, BOOST_SCOPED_ENUM(order) Order>
inline void endian_store( unsigned char * p, T const & v ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8 );
    BOOST_STATIC_ASSERT( N >= 1 && N <= sizeof(T) );

    return detail::endian_store_impl<T, sizeof(T), order::native, N, Order>()( p, v );
}

namespace detail
{

// same endianness, same size

template<class T, std::size_t N, BOOST_SCOPED_ENUM(order) O> struct endian_store_impl<T, N, O, N, O>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_trivially_copyable<T>::value );

        std::memcpy( p, &v, N );
    }
};

// same size, reverse endianness

template<class T, std::size_t N, BOOST_SCOPED_ENUM(order) O1, BOOST_SCOPED_ENUM(order) O2> struct endian_store_impl<T, N, O1, N, O2>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_trivially_copyable<T>::value );

        typename integral_by_size<N>::type tmp;
        std::memcpy( &tmp, &v, N );

        endian_reverse_inplace( tmp );

        std::memcpy( p, &tmp, N );
    }
};

// truncating store 2 -> 1

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 2, Order, 1, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 2 ];
        boost::endian::endian_store<T, 2, order::little>( tmp, v );

        p[0] = tmp[0];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 2, Order, 1, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 2 ];
        boost::endian::endian_store<T, 2, order::big>( tmp, v );

        p[0] = tmp[1];
    }
};

// truncating store 4 -> 1

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 1, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::little>( tmp, v );

        p[0] = tmp[0];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 1, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::big>( tmp, v );

        p[0] = tmp[3];
    }
};

// truncating store 4 -> 2

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 2, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 2, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::big>( tmp, v );

        p[0] = tmp[2];
        p[1] = tmp[3];
    }
};

// truncating store 4 -> 3

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 3, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 4, Order, 3, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 4 ];
        boost::endian::endian_store<T, 4, order::big>( tmp, v );

        p[0] = tmp[1];
        p[1] = tmp[2];
        p[2] = tmp[3];
    }
};

// truncating store 8 -> 1

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 1, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 1, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[7];
    }
};

// truncating store 8 -> 2

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 2, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 2, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[6];
        p[1] = tmp[7];
    }
};

// truncating store 8 -> 3

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 3, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 3, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[5];
        p[1] = tmp[6];
        p[2] = tmp[7];
    }
};

// truncating store 8 -> 4

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 4, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
        p[3] = tmp[3];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 4, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[4];
        p[1] = tmp[5];
        p[2] = tmp[6];
        p[3] = tmp[7];
    }
};

// truncating store 8 -> 5

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 5, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
        p[3] = tmp[3];
        p[4] = tmp[4];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 5, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[3];
        p[1] = tmp[4];
        p[2] = tmp[5];
        p[3] = tmp[6];
        p[4] = tmp[7];
    }
};

// truncating store 8 -> 6

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 6, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
        p[3] = tmp[3];
        p[4] = tmp[4];
        p[5] = tmp[5];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 6, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[2];
        p[1] = tmp[3];
        p[2] = tmp[4];
        p[3] = tmp[5];
        p[4] = tmp[6];
        p[5] = tmp[7];
    }
};

// truncating store 8 -> 7

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 7, order::little>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::little>( tmp, v );

        p[0] = tmp[0];
        p[1] = tmp[1];
        p[2] = tmp[2];
        p[3] = tmp[3];
        p[4] = tmp[4];
        p[5] = tmp[5];
        p[6] = tmp[6];
    }
};

template<class T, BOOST_SCOPED_ENUM(order) Order> struct endian_store_impl<T, 8, Order, 7, order::big>
{
    inline void operator()( unsigned char * p, T const & v ) const BOOST_NOEXCEPT
    {
        BOOST_STATIC_ASSERT( is_integral<T>::value || is_enum<T>::value );

        unsigned char tmp[ 8 ];
        boost::endian::endian_store<T, 8, order::big>( tmp, v );

        p[0] = tmp[1];
        p[1] = tmp[2];
        p[2] = tmp[3];
        p[3] = tmp[4];
        p[4] = tmp[5];
        p[5] = tmp[6];
        p[6] = tmp[7];
    }
};

} // namespace detail

} // namespace endian
} // namespace boost

#endif  // BOOST_ENDIAN_DETAIL_ENDIAN_STORE_HPP_INCLUDED
