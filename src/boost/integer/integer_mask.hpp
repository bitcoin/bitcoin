//  Boost integer/integer_mask.hpp header file  ------------------------------//

//  (C) Copyright Daryle Walker 2001.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

//  See https://www.boost.org for updates, documentation, and revision history. 

#ifndef BOOST_INTEGER_INTEGER_MASK_HPP
#define BOOST_INTEGER_INTEGER_MASK_HPP

#include <boost/integer_fwd.hpp>  // self include

#include <boost/config.hpp>   // for BOOST_STATIC_CONSTANT
#include <boost/integer.hpp>  // for boost::uint_t

#include <climits>  // for UCHAR_MAX, etc.
#include <cstddef>  // for std::size_t

#include <boost/limits.hpp>  // for std::numeric_limits

//
// We simply cannot include this header on gcc without getting copious warnings of the kind:
//
// boost/integer/integer_mask.hpp:93:35: warning: use of C99 long long integer constant
//
// And yet there is no other reasonable implementation, so we declare this a system header
// to suppress these warnings.
//
#if defined(__GNUC__) && (__GNUC__ >= 4)
#pragma GCC system_header
#endif

namespace boost
{


//  Specified single-bit mask class declaration  -----------------------------//
//  (Lowest bit starts counting at 0.)

template < std::size_t Bit >
struct high_bit_mask_t
{
    typedef typename uint_t<(Bit + 1)>::least  least;
    typedef typename uint_t<(Bit + 1)>::fast   fast;

    BOOST_STATIC_CONSTANT( least, high_bit = (least( 1u ) << Bit) );
    BOOST_STATIC_CONSTANT( fast, high_bit_fast = (fast( 1u ) << Bit) );

    BOOST_STATIC_CONSTANT( std::size_t, bit_position = Bit );

};  // boost::high_bit_mask_t


//  Specified bit-block mask class declaration  ------------------------------//
//  Makes masks for the lowest N bits
//  (Specializations are needed when N fills up a type.)

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4310)  // cast truncates constant value
#endif

template < std::size_t Bits >
struct low_bits_mask_t
{
    typedef typename uint_t<Bits>::least  least;
    typedef typename uint_t<Bits>::fast   fast;

    BOOST_STATIC_CONSTANT( least, sig_bits = least(~(least(~(least( 0u ))) << Bits )) );
    BOOST_STATIC_CONSTANT( fast, sig_bits_fast = fast(sig_bits) );

    BOOST_STATIC_CONSTANT( std::size_t, bit_count = Bits );

};  // boost::low_bits_mask_t

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#define BOOST_LOW_BITS_MASK_SPECIALIZE( Type )                                  \
  template <  >  struct low_bits_mask_t< std::numeric_limits<Type>::digits >  { \
      typedef std::numeric_limits<Type>           limits_type;                  \
      typedef uint_t<limits_type::digits>::least  least;                        \
      typedef uint_t<limits_type::digits>::fast   fast;                         \
      BOOST_STATIC_CONSTANT( least, sig_bits = (~( least(0u) )) );              \
      BOOST_STATIC_CONSTANT( fast, sig_bits_fast = fast(sig_bits) );            \
      BOOST_STATIC_CONSTANT( std::size_t, bit_count = limits_type::digits );    \
  }

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4245)  // 'initializing' : conversion from 'int' to 'const boost::low_bits_mask_t<8>::least', signed/unsigned mismatch
#endif

BOOST_LOW_BITS_MASK_SPECIALIZE( unsigned char );

#if USHRT_MAX > UCHAR_MAX
BOOST_LOW_BITS_MASK_SPECIALIZE( unsigned short );
#endif

#if UINT_MAX > USHRT_MAX
BOOST_LOW_BITS_MASK_SPECIALIZE( unsigned int );
#endif

#if ULONG_MAX > UINT_MAX
BOOST_LOW_BITS_MASK_SPECIALIZE( unsigned long );
#endif

#if defined(BOOST_HAS_LONG_LONG)
    #if ((defined(ULLONG_MAX) && (ULLONG_MAX > ULONG_MAX)) ||\
        (defined(ULONG_LONG_MAX) && (ULONG_LONG_MAX > ULONG_MAX)) ||\
        (defined(ULONGLONG_MAX) && (ULONGLONG_MAX > ULONG_MAX)) ||\
        (defined(_ULLONG_MAX) && (_ULLONG_MAX > ULONG_MAX)))
    BOOST_LOW_BITS_MASK_SPECIALIZE( boost::ulong_long_type );
    #endif
#elif defined(BOOST_HAS_MS_INT64)
    #if 18446744073709551615ui64 > ULONG_MAX
    BOOST_LOW_BITS_MASK_SPECIALIZE( unsigned __int64 );
    #endif
#endif

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#undef BOOST_LOW_BITS_MASK_SPECIALIZE


}  // namespace boost


#endif  // BOOST_INTEGER_INTEGER_MASK_HPP
