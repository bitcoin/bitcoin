#ifndef BOOST_CORE_BIT_HPP_INCLUDED
#define BOOST_CORE_BIT_HPP_INCLUDED

// MS compatible compilers support #pragma once

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// boost/core/bit.hpp
//
// A portable version of the C++20 standard header <bit>
//
// Copyright 2020 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <boost/config.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <limits>
#include <cstring>

#if defined(_MSC_VER)
# include <intrin.h>
# pragma intrinsic(_BitScanForward)
# pragma intrinsic(_BitScanReverse)
# if defined(_M_X64)
#  pragma intrinsic(_BitScanForward64)
#  pragma intrinsic(_BitScanReverse64)
# endif
#endif // defined(_MSC_VER)

namespace boost
{
namespace core
{

// bit_cast

template<class To, class From>
To bit_cast( From const & from ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(To) == sizeof(From) );

    To to;
    std::memcpy( &to, &from, sizeof(To) );
    return to;
}

// countl

#if defined(__GNUC__) || defined(__clang__)

namespace detail
{

BOOST_CONSTEXPR inline int countl_impl( unsigned char x ) BOOST_NOEXCEPT
{
    return x? __builtin_clz( x ) - ( std::numeric_limits<unsigned int>::digits - std::numeric_limits<unsigned char>::digits ): std::numeric_limits<unsigned char>::digits;
}

BOOST_CONSTEXPR inline int countl_impl( unsigned short x ) BOOST_NOEXCEPT
{
    return x? __builtin_clz( x ) - ( std::numeric_limits<unsigned int>::digits - std::numeric_limits<unsigned short>::digits ): std::numeric_limits<unsigned short>::digits;
}

BOOST_CONSTEXPR inline int countl_impl( unsigned int x ) BOOST_NOEXCEPT
{
    return x? __builtin_clz( x ): std::numeric_limits<unsigned int>::digits;
}

BOOST_CONSTEXPR inline int countl_impl( unsigned long x ) BOOST_NOEXCEPT
{
    return x? __builtin_clzl( x ): std::numeric_limits<unsigned long>::digits;
}

BOOST_CONSTEXPR inline int countl_impl( unsigned long long x ) BOOST_NOEXCEPT
{
    return x? __builtin_clzll( x ): std::numeric_limits<unsigned long long>::digits;
}

} // namespace detail

template<class T>
BOOST_CONSTEXPR int countl_zero( T x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countl_impl( x );
}

#else // defined(__GNUC__) || defined(__clang__)

namespace detail
{

inline int countl_impl( boost::uint32_t x ) BOOST_NOEXCEPT
{
#if defined(_MSC_VER)

    unsigned long r;

    if( _BitScanReverse( &r, x ) )
    {
        return 31 - static_cast<int>( r );
    }
    else
    {
        return 32;
    }

#else

    static unsigned char const mod37[ 37 ] = { 32, 31, 6, 30, 9, 5, 0, 29, 16, 8, 2, 4, 21, 0, 19, 28, 25, 15, 0, 7, 10, 1, 17, 3, 22, 20, 26, 0, 11, 18, 23, 27, 12, 24, 13, 14, 0 };

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    return mod37[ x % 37 ];

#endif
}

inline int countl_impl( boost::uint64_t x ) BOOST_NOEXCEPT
{
#if defined(_MSC_VER) && defined(_M_X64)

    unsigned long r;

    if( _BitScanReverse64( &r, x ) )
    {
        return 63 - static_cast<int>( r );
    }
    else
    {
        return 64;
    }

#else

    return static_cast<boost::uint32_t>( x >> 32 ) != 0?
        boost::core::detail::countl_impl( static_cast<boost::uint32_t>( x >> 32 ) ):
        boost::core::detail::countl_impl( static_cast<boost::uint32_t>( x ) ) + 32;

#endif
}

inline int countl_impl( boost::uint8_t x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countl_impl( static_cast<boost::uint32_t>( x ) ) - 24;
}

inline int countl_impl( boost::uint16_t x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countl_impl( static_cast<boost::uint32_t>( x ) ) - 16;
}

} // namespace detail

template<class T>
int countl_zero( T x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(T) == sizeof(boost::uint8_t) || sizeof(T) == sizeof(boost::uint16_t) || sizeof(T) == sizeof(boost::uint32_t) || sizeof(T) == sizeof(boost::uint64_t) );

    if( sizeof(T) == sizeof(boost::uint8_t) )
    {
        return boost::core::detail::countl_impl( static_cast<boost::uint8_t>( x ) );
    }
    else if( sizeof(T) == sizeof(boost::uint16_t) )
    {
        return boost::core::detail::countl_impl( static_cast<boost::uint16_t>( x ) );
    }
    else if( sizeof(T) == sizeof(boost::uint32_t) )
    {
        return boost::core::detail::countl_impl( static_cast<boost::uint32_t>( x ) );
    }
    else
    {
        return boost::core::detail::countl_impl( static_cast<boost::uint64_t>( x ) );
    }
}

#endif // defined(__GNUC__) || defined(__clang__)

template<class T>
BOOST_CONSTEXPR int countl_one( T x ) BOOST_NOEXCEPT
{
    return boost::core::countl_zero( static_cast<T>( ~x ) );
}

// countr

#if defined(__GNUC__) || defined(__clang__)

namespace detail
{

BOOST_CONSTEXPR inline int countr_impl( unsigned char x ) BOOST_NOEXCEPT
{
    return x? __builtin_ctz( x ): std::numeric_limits<unsigned char>::digits;
}

BOOST_CONSTEXPR inline int countr_impl( unsigned short x ) BOOST_NOEXCEPT
{
    return x? __builtin_ctz( x ): std::numeric_limits<unsigned short>::digits;
}

BOOST_CONSTEXPR inline int countr_impl( unsigned int x ) BOOST_NOEXCEPT
{
    return x? __builtin_ctz( x ): std::numeric_limits<unsigned int>::digits;
}

BOOST_CONSTEXPR inline int countr_impl( unsigned long x ) BOOST_NOEXCEPT
{
    return x? __builtin_ctzl( x ): std::numeric_limits<unsigned long>::digits;
}

BOOST_CONSTEXPR inline int countr_impl( unsigned long long x ) BOOST_NOEXCEPT
{
    return x? __builtin_ctzll( x ): std::numeric_limits<unsigned long long>::digits;
}

} // namespace detail

template<class T>
BOOST_CONSTEXPR int countr_zero( T x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countr_impl( x );
}

#else // defined(__GNUC__) || defined(__clang__)

namespace detail
{

inline int countr_impl( boost::uint32_t x ) BOOST_NOEXCEPT
{
#if defined(_MSC_VER)

    unsigned long r;

    if( _BitScanForward( &r, x ) )
    {
        return static_cast<int>( r );
    }
    else
    {
        return 32;
    }

#else

    static unsigned char const mod37[ 37 ] = { 32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4, 7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5, 20, 8, 19, 18 };
    return mod37[ ( -(boost::int32_t)x & x ) % 37 ];

#endif
}

inline int countr_impl( boost::uint64_t x ) BOOST_NOEXCEPT
{
#if defined(_MSC_VER) && defined(_M_X64)

    unsigned long r;

    if( _BitScanForward64( &r, x ) )
    {
        return static_cast<int>( r );
    }
    else
    {
        return 64;
    }

#else

    return static_cast<boost::uint32_t>( x ) != 0?
        boost::core::detail::countr_impl( static_cast<boost::uint32_t>( x ) ):
        boost::core::detail::countr_impl( static_cast<boost::uint32_t>( x >> 32 ) ) + 32;

#endif
}

inline int countr_impl( boost::uint8_t x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countr_impl( static_cast<boost::uint32_t>( x ) | 0x100 );
}

inline int countr_impl( boost::uint16_t x ) BOOST_NOEXCEPT
{
    return boost::core::detail::countr_impl( static_cast<boost::uint32_t>( x ) | 0x10000 );
}

} // namespace detail

template<class T>
int countr_zero( T x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(T) == sizeof(boost::uint8_t) || sizeof(T) == sizeof(boost::uint16_t) || sizeof(T) == sizeof(boost::uint32_t) || sizeof(T) == sizeof(boost::uint64_t) );

    if( sizeof(T) == sizeof(boost::uint8_t) )
    {
        return boost::core::detail::countr_impl( static_cast<boost::uint8_t>( x ) );
    }
    else if( sizeof(T) == sizeof(boost::uint16_t) )
    {
        return boost::core::detail::countr_impl( static_cast<boost::uint16_t>( x ) );
    }
    else if( sizeof(T) == sizeof(boost::uint32_t) )
    {
        return boost::core::detail::countr_impl( static_cast<boost::uint32_t>( x ) );
    }
    else
    {
        return boost::core::detail::countr_impl( static_cast<boost::uint64_t>( x ) );
    }
}

#endif // defined(__GNUC__) || defined(__clang__)

template<class T>
BOOST_CONSTEXPR int countr_one( T x ) BOOST_NOEXCEPT
{
    return boost::core::countr_zero( static_cast<T>( ~x ) );
}

// popcount

#if defined(__GNUC__) || defined(__clang__)

#if defined(__clang__) && __clang_major__ * 100 + __clang_minor__ < 304
# define BOOST_CORE_POPCOUNT_CONSTEXPR
#else
# define BOOST_CORE_POPCOUNT_CONSTEXPR BOOST_CONSTEXPR
#endif

namespace detail
{

BOOST_CORE_POPCOUNT_CONSTEXPR inline int popcount_impl( unsigned char x ) BOOST_NOEXCEPT
{
    return __builtin_popcount( x );
}

BOOST_CORE_POPCOUNT_CONSTEXPR inline int popcount_impl( unsigned short x ) BOOST_NOEXCEPT
{
    return __builtin_popcount( x );
}

BOOST_CORE_POPCOUNT_CONSTEXPR inline int popcount_impl( unsigned int x ) BOOST_NOEXCEPT
{
    return __builtin_popcount( x );
}

BOOST_CORE_POPCOUNT_CONSTEXPR inline int popcount_impl( unsigned long x ) BOOST_NOEXCEPT
{
    return __builtin_popcountl( x );
}

BOOST_CORE_POPCOUNT_CONSTEXPR inline int popcount_impl( unsigned long long x ) BOOST_NOEXCEPT
{
    return __builtin_popcountll( x );
}

} // namespace detail

#undef BOOST_CORE_POPCOUNT_CONSTEXPR

template<class T>
BOOST_CONSTEXPR int popcount( T x ) BOOST_NOEXCEPT
{
    return boost::core::detail::popcount_impl( x );
}

#else // defined(__GNUC__) || defined(__clang__)

namespace detail
{

BOOST_CXX14_CONSTEXPR inline int popcount_impl( boost::uint32_t x ) BOOST_NOEXCEPT
{
    x = x - ( ( x >> 1 ) & 0x55555555 );
    x = ( x & 0x33333333 ) + ( ( x >> 2 ) & 0x33333333 );
    x = ( x + ( x >> 4 ) ) & 0x0F0F0F0F;

    return static_cast<unsigned>( ( x * 0x01010101 ) >> 24 );
}

BOOST_CXX14_CONSTEXPR inline int popcount_impl( boost::uint64_t x ) BOOST_NOEXCEPT
{
    x = x - ( ( x >> 1 ) & 0x5555555555555555 );
    x = ( x & 0x3333333333333333 ) + ( ( x >> 2 ) & 0x3333333333333333 );
    x = ( x + ( x >> 4 ) ) & 0x0F0F0F0F0F0F0F0F;

    return static_cast<unsigned>( ( x * 0x0101010101010101 ) >> 56 );
}

} // namespace detail

template<class T>
BOOST_CXX14_CONSTEXPR int popcount( T x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(T) <= sizeof(boost::uint64_t) );

    if( sizeof(T) <= sizeof(boost::uint32_t) )
    {
        return boost::core::detail::popcount_impl( static_cast<boost::uint32_t>( x ) );
    }
    else
    {
        return boost::core::detail::popcount_impl( static_cast<boost::uint64_t>( x ) );
    }
}

#endif // defined(__GNUC__) || defined(__clang__)

// rotating

template<class T>
BOOST_CXX14_CONSTEXPR T rotl( T x, int s ) BOOST_NOEXCEPT
{
    unsigned const mask = std::numeric_limits<T>::digits - 1;
    return x << (s & mask) | x >> ((-s) & mask);
}

template<class T>
BOOST_CXX14_CONSTEXPR T rotr( T x, int s ) BOOST_NOEXCEPT
{
    unsigned const mask = std::numeric_limits<T>::digits - 1;
    return x >> (s & mask) | x << ((-s) & mask);
}

// integral powers of 2

template<class T>
BOOST_CONSTEXPR bool has_single_bit( T x ) BOOST_NOEXCEPT
{
    return x != 0 && ( x & ( x - 1 ) ) == 0;
}

template<class T>
BOOST_CONSTEXPR T bit_width( T x ) BOOST_NOEXCEPT
{
    return std::numeric_limits<T>::digits - boost::core::countl_zero( x );
}

template<class T>
BOOST_CONSTEXPR T bit_floor( T x ) BOOST_NOEXCEPT
{
    return x == 0? 0: T(1) << ( boost::core::bit_width( x ) - 1 );
}

namespace detail
{

BOOST_CXX14_CONSTEXPR inline boost::uint32_t bit_ceil_impl( boost::uint32_t x ) BOOST_NOEXCEPT
{
    if( x == 0 )
    {
        return 0;
    }

    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    ++x;

    return x;
}

BOOST_CXX14_CONSTEXPR inline boost::uint64_t bit_ceil_impl( boost::uint64_t x ) BOOST_NOEXCEPT
{
    if( x == 0 )
    {
        return 0;
    }

    --x;

    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    ++x;

    return x;
}

} // namespace detail

template<class T>
BOOST_CXX14_CONSTEXPR T bit_ceil( T x ) BOOST_NOEXCEPT
{
    BOOST_STATIC_ASSERT( sizeof(T) <= sizeof(boost::uint64_t) );

    if( sizeof(T) <= sizeof(boost::uint32_t) )
    {
        return static_cast<T>( boost::core::detail::bit_ceil_impl( static_cast<boost::uint32_t>( x ) ) );
    }
    else
    {
        return static_cast<T>( boost::core::detail::bit_ceil_impl( static_cast<boost::uint64_t>( x ) ) );
    }
}

// endian

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

# define BOOST_CORE_BIT_NATIVE_INITIALIZER =little

#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

# define BOOST_CORE_BIT_NATIVE_INITIALIZER =big

#elif defined(__BYTE_ORDER__) && defined(__ORDER_PDP_ENDIAN__) && __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__

# define BOOST_CORE_BIT_NATIVE_INITIALIZER

#elif defined(__LITTLE_ENDIAN__)

# define BOOST_CORE_BIT_NATIVE_INITIALIZER =little

#elif defined(__BIG_ENDIAN__)

# define BOOST_CORE_BIT_NATIVE_INITIALIZER =big

#elif defined(_MSC_VER) || defined(__i386__) || defined(__x86_64__)

# define BOOST_CORE_BIT_NATIVE_INITIALIZER =little

#else

# define BOOST_CORE_BIT_NATIVE_INITIALIZER

#endif

#if !defined(BOOST_NO_CXX11_SCOPED_ENUMS)

enum class endian
{
    big,
    little,
    native BOOST_CORE_BIT_NATIVE_INITIALIZER
};

typedef endian endian_type;

#else

namespace endian
{

enum type
{
    big,
    little,
    native BOOST_CORE_BIT_NATIVE_INITIALIZER
};

} // namespace endian

typedef endian::type endian_type;

#endif

#undef BOOST_CORE_BIT_NATIVE_INITIALIZER

} // namespace core
} // namespace boost

#endif  // #ifndef BOOST_CORE_BIT_HPP_INCLUDED
