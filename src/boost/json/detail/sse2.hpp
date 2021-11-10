//
// Copyright (c) 2019 Peter Dimov (pdimov at gmail dot com),
//                    Vinnie Falco (vinnie.falco@gmail.com)
// Copyright (c) 2020 Krystian Stasiowski (sdkrystian@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_SSE2_HPP
#define BOOST_JSON_DETAIL_SSE2_HPP

#include <boost/json/detail/config.hpp>
#include <boost/json/detail/utf8.hpp>
#include <cstddef>
#include <cstring>
#ifdef BOOST_JSON_USE_SSE2
# include <emmintrin.h>
# include <xmmintrin.h>
# ifdef _MSC_VER
#  include <intrin.h>
# endif
#endif

BOOST_JSON_NS_BEGIN
namespace detail {

#ifdef BOOST_JSON_USE_SSE2

template<bool AllowBadUTF8>
inline
const char*
count_valid(
    char const* p,
    const char* end) noexcept
{
    __m128i const q1 = _mm_set1_epi8( '\x22' ); // '"'
    __m128i const q2 = _mm_set1_epi8( '\\' ); // '\\'
    __m128i const q3 = _mm_set1_epi8( 0x1F );

    while(end - p >= 16)
    {
        __m128i v1 = _mm_loadu_si128( (__m128i const*)p );
        __m128i v2 = _mm_cmpeq_epi8( v1, q1 ); // quote
        __m128i v3 = _mm_cmpeq_epi8( v1, q2 ); // backslash
        __m128i v4 = _mm_or_si128( v2, v3 ); // combine quotes and backslash
        __m128i v5 = _mm_min_epu8( v1, q3 );
        __m128i v6 = _mm_cmpeq_epi8( v5, v1 ); // controls
        __m128i v7 = _mm_or_si128( v4, v6 ); // combine with control

        int w = _mm_movemask_epi8( v7 );

        if( w != 0 )
        {
            int m;
#if defined(__GNUC__) || defined(__clang__)
            m = __builtin_ffs( w ) - 1;
#else
            unsigned long index;
            _BitScanForward( &index, w );
            m = index;
#endif
            return p + m;
        }

        p += 16;
    }

    while(p != end)
    {
        const unsigned char c = *p;
        if(c == '\x22' || c == '\\' || c < 0x20)
            break;
        ++p;
    }

    return p;
}

template<>
inline
const char*
count_valid<false>(
    char const* p,
    const char* end) noexcept
{
    __m128i const q1 = _mm_set1_epi8( '\x22' ); // '"'
    __m128i const q2 = _mm_set1_epi8( '\\' );
    __m128i const q3 = _mm_set1_epi8( 0x20 );

    while(end - p >= 16)
    {
        __m128i v1 = _mm_loadu_si128( (__m128i const*)p );

        __m128i v2 = _mm_cmpeq_epi8( v1, q1 );
        __m128i v3 = _mm_cmpeq_epi8( v1, q2 );
        __m128i v4 = _mm_cmplt_epi8( v1, q3 );

        __m128i v5 = _mm_or_si128( v2, v3 );
        __m128i v6 = _mm_or_si128( v5, v4 );

        int w = _mm_movemask_epi8( v6 );

        if( w != 0 )
        {
            int m;
#if defined(__GNUC__) || defined(__clang__)
            m = __builtin_ffs( w ) - 1;
#else
            unsigned long index;
            _BitScanForward( &index, w );
            m = index;
#endif
            p += m;
            break;
        }

        p += 16;
    }

    while(p != end)
    {
        const unsigned char c = *p;
        if(c == '\x22' || c == '\\' || c < 0x20)
            break;
        if(c < 0x80)
        {
            ++p;
            continue;
        }
        // validate utf-8
        uint16_t first = classify_utf8(c & 0x7F);
        uint8_t len = first & 0xFF;
        if(BOOST_JSON_UNLIKELY(end - p < len))
            break;
        if(BOOST_JSON_UNLIKELY(! is_valid_utf8(p, first)))
            break;
        p += len;
    }

    return p;
}

#else

template<bool AllowBadUTF8>
char const*
count_valid(
    char const* p,
    char const* end) noexcept
{
    while(p != end)
    {
        const unsigned char c = *p;
        if(c == '\x22' || c == '\\' || c < 0x20)
            break;
        ++p;
    }

    return p;
}

template<>
inline
char const*
count_valid<false>(
    char const* p,
    char const* end) noexcept
{
    while(p != end)
    {
        const unsigned char c = *p;
        if(c == '\x22' || c == '\\' || c < 0x20)
            break;
        if(c < 0x80)
        {
            ++p;
            continue;
        }
        // validate utf-8
        uint16_t first = classify_utf8(c & 0x7F);
        uint8_t len = first & 0xFF;
        if(BOOST_JSON_UNLIKELY(end - p < len))
            break;
        if(BOOST_JSON_UNLIKELY(! is_valid_utf8(p, first)))
            break;
        p += len;
    }

    return p;
}

#endif

// KRYSTIAN NOTE: does not stop to validate
// count_unescaped

#ifdef BOOST_JSON_USE_SSE2

inline
size_t
count_unescaped(
    char const* s,
    size_t n) noexcept
{

    __m128i const q1 = _mm_set1_epi8( '\x22' ); // '"'
    __m128i const q2 = _mm_set1_epi8( '\\' ); // '\\'
    __m128i const q3 = _mm_set1_epi8( 0x1F );

    char const * s0 = s;

    while( n >= 16 )
    {
        __m128i v1 = _mm_loadu_si128( (__m128i const*)s );
        __m128i v2 = _mm_cmpeq_epi8( v1, q1 ); // quote
        __m128i v3 = _mm_cmpeq_epi8( v1, q2 ); // backslash
        __m128i v4 = _mm_or_si128( v2, v3 ); // combine quotes and backslash
        __m128i v5 = _mm_min_epu8( v1, q3 );
        __m128i v6 = _mm_cmpeq_epi8( v5, v1 ); // controls
        __m128i v7 = _mm_or_si128( v4, v6 ); // combine with control

        int w = _mm_movemask_epi8( v7 );

        if( w != 0 )
        {
            int m;
#if defined(__GNUC__) || defined(__clang__)
            m = __builtin_ffs( w ) - 1;
#else
            unsigned long index;
            _BitScanForward( &index, w );
            m = index;
#endif

            s += m;
            break;
        }

        s += 16;
        n -= 16;
    }

    return s - s0;
}

#else

inline
std::size_t
count_unescaped(
    char const*,
    std::size_t) noexcept
{
    return 0;
}

#endif

// count_digits

#ifdef BOOST_JSON_USE_SSE2

// assumes p..p+15 are valid
inline int count_digits( char const* p ) noexcept
{
    __m128i v1 = _mm_loadu_si128( (__m128i const*)p );
    v1 = _mm_add_epi8(v1, _mm_set1_epi8(70));
    v1 = _mm_cmplt_epi8(v1, _mm_set1_epi8(118));

    int m = _mm_movemask_epi8(v1);

    int n;

    if( m == 0 )
    {
        n = 16;
    }
    else
    {
#if defined(__GNUC__) || defined(__clang__)
        n = __builtin_ffs( m ) - 1;
#else
        unsigned long index;
        _BitScanForward( &index, m );
        n = static_cast<int>(index);
#endif
    }

    return n;
}

#else

// assumes p..p+15 are valid
inline int count_digits( char const* p ) noexcept
{
    int n = 0;

    for( ; n < 16; ++n )
    {
        unsigned char const d = *p++ - '0';
        if(d > 9) break;
    }

    return n;
}

#endif

// parse_unsigned

inline uint64_t parse_unsigned( uint64_t r, char const * p, std::size_t n ) noexcept
{
    while( n >= 4 )
    {
        // faster on on clang for x86,
        // slower on gcc
#ifdef __clang__
        r = r * 10 + p[0] - '0';
        r = r * 10 + p[1] - '0';
        r = r * 10 + p[2] - '0';
        r = r * 10 + p[3] - '0';
#else
        uint32_t v;
        std::memcpy( &v, p, 4 );

        v -= 0x30303030;

        unsigned w0 = v & 0xFF;
        unsigned w1 = (v >> 8) & 0xFF;
        unsigned w2 = (v >> 16) & 0xFF;
        unsigned w3 = (v >> 24);

#ifdef BOOST_JSON_BIG_ENDIAN
        r = (((r * 10 + w3) * 10 + w2) * 10 + w1) * 10 + w0;
#else
        r = (((r * 10 + w0) * 10 + w1) * 10 + w2) * 10 + w3;
#endif
#endif
        p += 4;
        n -= 4;
    }

    switch( n )
    {
    case 0:
        break;
    case 1:
        r = r * 10 + p[0] - '0';
        break;
    case 2:
        r = r * 10 + p[0] - '0';
        r = r * 10 + p[1] - '0';
        break;
    case 3:
        r = r * 10 + p[0] - '0';
        r = r * 10 + p[1] - '0';
        r = r * 10 + p[2] - '0';
        break;
    }
    return r;
}

// KRYSTIAN: this function is unused
// count_leading

/*
#ifdef BOOST_JSON_USE_SSE2

// assumes p..p+15
inline std::size_t count_leading( char const * p, char ch ) noexcept
{
    __m128i const q1 = _mm_set1_epi8( ch );

    __m128i v = _mm_loadu_si128( (__m128i const*)p );

    __m128i w = _mm_cmpeq_epi8( v, q1 );

    int m = _mm_movemask_epi8( w ) ^ 0xFFFF;

    std::size_t n;

    if( m == 0 )
    {
        n = 16;
    }
    else
    {
#if defined(__GNUC__) || defined(__clang__)
        n = __builtin_ffs( m ) - 1;
#else
        unsigned long index;
        _BitScanForward( &index, m );
        n = index;
#endif
    }

    return n;
}

#else

// assumes p..p+15
inline std::size_t count_leading( char const * p, char ch ) noexcept
{
    std::size_t n = 0;

    for( ; n < 16 && *p == ch; ++p, ++n );

    return n;
}

#endif
*/

// count_whitespace

#ifdef BOOST_JSON_USE_SSE2

inline const char* count_whitespace( char const* p, const char* end ) noexcept
{
    if( p == end )
    {
        return p;
    }

    if( static_cast<unsigned char>( *p ) > 0x20 )
    {
        return p;
    }

    __m128i const q1 = _mm_set1_epi8( ' ' );
    __m128i const q2 = _mm_set1_epi8( '\n' );
    __m128i const q3 = _mm_set1_epi8( 4 ); // '\t' | 4 == '\r'
    __m128i const q4 = _mm_set1_epi8( '\r' );

    while( end - p >= 16 )
    {
        __m128i v0 = _mm_loadu_si128( (__m128i const*)p );

        __m128i w0 = _mm_or_si128(
            _mm_cmpeq_epi8( v0, q1 ),
            _mm_cmpeq_epi8( v0, q2 ));
        __m128i v1 = _mm_or_si128( v0, q3 );
        __m128i w1 = _mm_cmpeq_epi8( v1, q4 );
        __m128i w2 = _mm_or_si128( w0, w1 );

        int m = _mm_movemask_epi8( w2 ) ^ 0xFFFF;

        if( m != 0 )
        {
#if defined(__GNUC__) || defined(__clang__)
            std::size_t c = __builtin_ffs( m ) - 1;
#else
            unsigned long index;
            _BitScanForward( &index, m );
            std::size_t c = index;
#endif

            p += c;
            return p;
        }

        p += 16;
    }

    while( p != end )
    {
        if( *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' )
        {
            return p;
        }

        ++p;
    }

    return p;
}

/*

// slightly faster on msvc-14.2, slightly slower on clang-win

inline std::size_t count_whitespace( char const * p, std::size_t n ) noexcept
{
    char const * p0 = p;

    while( n > 0 )
    {
        char ch = *p;

        if( ch == '\n' || ch == '\r' )
        {
            ++p;
            --n;
            continue;
        }

        if( ch != ' ' && ch != '\t' )
        {
            break;
        }

        ++p;
        --n;

        while( n >= 16 )
        {
            std::size_t n2 = count_leading( p, ch );

            p += n2;
            n -= n2;

            if( n2 < 16 )
            {
                break;
            }
        }
    }

    return p - p0;
}
*/

#else

inline const char* count_whitespace( char const* p, const char* end ) noexcept
{

    for(; p != end; ++p)
    {
        char const c = *p;
        if( c != ' ' && c != '\n' && c != '\r' && c != '\t' ) break;
    }

    return p;
}

#endif

} // detail
BOOST_JSON_NS_END

#endif
