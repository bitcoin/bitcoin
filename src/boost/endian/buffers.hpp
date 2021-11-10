//  boost/endian/buffers.hpp  ----------------------------------------------------------//

//  (C) Copyright Darin Adler 2000
//  (C) Copyright Beman Dawes 2006, 2009, 2014
//  (C) Copyright Peter Dimov 2019

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See library home page at http://www.boost.org/libs/endian

//--------------------------------------------------------------------------------------//

//  Original design developed by Darin Adler based on classes developed by Mark
//  Borgerding. Four original class templates were combined into a single endian
//  class template by Beman Dawes, who also added the unrolled_byte_loops sign
//  partial specialization to correctly extend the sign when cover integer size
//  differs from endian representation size.

// TODO: When a compiler supporting constexpr becomes available, try possible uses.

#ifndef BOOST_ENDIAN_BUFFERS_HPP
#define BOOST_ENDIAN_BUFFERS_HPP

#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4127)  // conditional expression is constant
#endif

#include <boost/endian/detail/endian_store.hpp>
#include <boost/endian/detail/endian_load.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/static_assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/config.hpp>
#include <boost/config/workaround.hpp>
#include <iosfwd>
#include <climits>
#include <cstring>

#if defined(BOOST_BORLANDC) || defined(BOOST_CODEGEARC)
# pragma pack(push, 1)
#endif

# if CHAR_BIT != 8
#   error Platforms with CHAR_BIT != 8 are not supported
# endif

# ifdef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS
#   define BOOST_ENDIAN_DEFAULT_CONSTRUCT {}          // C++03
# else
#   define BOOST_ENDIAN_DEFAULT_CONSTRUCT = default;  // C++0x
# endif

// g++ pre-4.6 does not support unrestricted unions, but we have no Config macro for that
# if (defined(BOOST_NO_CXX11_DEFAULTED_FUNCTIONS) || BOOST_WORKAROUND(BOOST_GCC, < 40600)) && defined(BOOST_ENDIAN_FORCE_PODNESS)
#   define BOOST_ENDIAN_NO_CTORS
# endif

//----------------------------------  synopsis  ----------------------------------------//

namespace boost
{
namespace endian
{

  BOOST_SCOPED_ENUM_START(align)
  {no, yes
#   ifdef BOOST_ENDIAN_DEPRECATED_NAMES
      , unaligned = no, aligned = yes
#   endif
  }; BOOST_SCOPED_ENUM_END

  template <BOOST_SCOPED_ENUM(order) Order, class T, std::size_t n_bits,
    BOOST_SCOPED_ENUM(align) A = align::no>
      class endian_buffer;

  // aligned big endian signed integer buffers
  typedef endian_buffer<order::big, int8_t, 8, align::yes>       big_int8_buf_at;
  typedef endian_buffer<order::big, int16_t, 16, align::yes>     big_int16_buf_at;
  typedef endian_buffer<order::big, int32_t, 32, align::yes>     big_int32_buf_at;
  typedef endian_buffer<order::big, int64_t, 64, align::yes>     big_int64_buf_at;

  // aligned big endian unsigned integer buffers
  typedef endian_buffer<order::big, uint8_t, 8, align::yes>      big_uint8_buf_at;
  typedef endian_buffer<order::big, uint16_t, 16, align::yes>    big_uint16_buf_at;
  typedef endian_buffer<order::big, uint32_t, 32, align::yes>    big_uint32_buf_at;
  typedef endian_buffer<order::big, uint64_t, 64, align::yes>    big_uint64_buf_at;

  // aligned little endian signed integer buffers
  typedef endian_buffer<order::little, int8_t, 8, align::yes>    little_int8_buf_at;
  typedef endian_buffer<order::little, int16_t, 16, align::yes>  little_int16_buf_at;
  typedef endian_buffer<order::little, int32_t, 32, align::yes>  little_int32_buf_at;
  typedef endian_buffer<order::little, int64_t, 64, align::yes>  little_int64_buf_at;

  // aligned little endian unsigned integer buffers
  typedef endian_buffer<order::little, uint8_t, 8, align::yes>   little_uint8_buf_at;
  typedef endian_buffer<order::little, uint16_t, 16, align::yes> little_uint16_buf_at;
  typedef endian_buffer<order::little, uint32_t, 32, align::yes> little_uint32_buf_at;
  typedef endian_buffer<order::little, uint64_t, 64, align::yes> little_uint64_buf_at;

  // aligned floating point buffers
  typedef endian_buffer<order::big, float, 32, align::yes>       big_float32_buf_at;
  typedef endian_buffer<order::big, double, 64, align::yes>      big_float64_buf_at;
  typedef endian_buffer<order::little, float, 32, align::yes>    little_float32_buf_at;
  typedef endian_buffer<order::little, double, 64, align::yes>   little_float64_buf_at;

  // aligned native endian typedefs are not provided because
  // <cstdint> types are superior for this use case

  // unaligned big endian signed integer buffers
  typedef endian_buffer<order::big, int_least8_t, 8>        big_int8_buf_t;
  typedef endian_buffer<order::big, int_least16_t, 16>      big_int16_buf_t;
  typedef endian_buffer<order::big, int_least32_t, 24>      big_int24_buf_t;
  typedef endian_buffer<order::big, int_least32_t, 32>      big_int32_buf_t;
  typedef endian_buffer<order::big, int_least64_t, 40>      big_int40_buf_t;
  typedef endian_buffer<order::big, int_least64_t, 48>      big_int48_buf_t;
  typedef endian_buffer<order::big, int_least64_t, 56>      big_int56_buf_t;
  typedef endian_buffer<order::big, int_least64_t, 64>      big_int64_buf_t;

  // unaligned big endian unsigned integer buffers
  typedef endian_buffer<order::big, uint_least8_t, 8>       big_uint8_buf_t;
  typedef endian_buffer<order::big, uint_least16_t, 16>     big_uint16_buf_t;
  typedef endian_buffer<order::big, uint_least32_t, 24>     big_uint24_buf_t;
  typedef endian_buffer<order::big, uint_least32_t, 32>     big_uint32_buf_t;
  typedef endian_buffer<order::big, uint_least64_t, 40>     big_uint40_buf_t;
  typedef endian_buffer<order::big, uint_least64_t, 48>     big_uint48_buf_t;
  typedef endian_buffer<order::big, uint_least64_t, 56>     big_uint56_buf_t;
  typedef endian_buffer<order::big, uint_least64_t, 64>     big_uint64_buf_t;

  // unaligned little endian signed integer buffers
  typedef endian_buffer<order::little, int_least8_t, 8>     little_int8_buf_t;
  typedef endian_buffer<order::little, int_least16_t, 16>   little_int16_buf_t;
  typedef endian_buffer<order::little, int_least32_t, 24>   little_int24_buf_t;
  typedef endian_buffer<order::little, int_least32_t, 32>   little_int32_buf_t;
  typedef endian_buffer<order::little, int_least64_t, 40>   little_int40_buf_t;
  typedef endian_buffer<order::little, int_least64_t, 48>   little_int48_buf_t;
  typedef endian_buffer<order::little, int_least64_t, 56>   little_int56_buf_t;
  typedef endian_buffer<order::little, int_least64_t, 64>   little_int64_buf_t;

  // unaligned little endian unsigned integer buffers
  typedef endian_buffer<order::little, uint_least8_t, 8>    little_uint8_buf_t;
  typedef endian_buffer<order::little, uint_least16_t, 16>  little_uint16_buf_t;
  typedef endian_buffer<order::little, uint_least32_t, 24>  little_uint24_buf_t;
  typedef endian_buffer<order::little, uint_least32_t, 32>  little_uint32_buf_t;
  typedef endian_buffer<order::little, uint_least64_t, 40>  little_uint40_buf_t;
  typedef endian_buffer<order::little, uint_least64_t, 48>  little_uint48_buf_t;
  typedef endian_buffer<order::little, uint_least64_t, 56>  little_uint56_buf_t;
  typedef endian_buffer<order::little, uint_least64_t, 64>  little_uint64_buf_t;

  // unaligned native endian signed integer buffers
  typedef endian_buffer<order::native, int_least8_t, 8>     native_int8_buf_t;
  typedef endian_buffer<order::native, int_least16_t, 16>   native_int16_buf_t;
  typedef endian_buffer<order::native, int_least32_t, 24>   native_int24_buf_t;
  typedef endian_buffer<order::native, int_least32_t, 32>   native_int32_buf_t;
  typedef endian_buffer<order::native, int_least64_t, 40>   native_int40_buf_t;
  typedef endian_buffer<order::native, int_least64_t, 48>   native_int48_buf_t;
  typedef endian_buffer<order::native, int_least64_t, 56>   native_int56_buf_t;
  typedef endian_buffer<order::native, int_least64_t, 64>   native_int64_buf_t;

  // unaligned native endian unsigned integer buffers
  typedef endian_buffer<order::native, uint_least8_t, 8>    native_uint8_buf_t;
  typedef endian_buffer<order::native, uint_least16_t, 16>  native_uint16_buf_t;
  typedef endian_buffer<order::native, uint_least32_t, 24>  native_uint24_buf_t;
  typedef endian_buffer<order::native, uint_least32_t, 32>  native_uint32_buf_t;
  typedef endian_buffer<order::native, uint_least64_t, 40>  native_uint40_buf_t;
  typedef endian_buffer<order::native, uint_least64_t, 48>  native_uint48_buf_t;
  typedef endian_buffer<order::native, uint_least64_t, 56>  native_uint56_buf_t;
  typedef endian_buffer<order::native, uint_least64_t, 64>  native_uint64_buf_t;

  // unaligned floating point buffers
  typedef endian_buffer<order::big, float, 32, align::no>       big_float32_buf_t;
  typedef endian_buffer<order::big, double, 64, align::no>      big_float64_buf_t;
  typedef endian_buffer<order::little, float, 32, align::no>    little_float32_buf_t;
  typedef endian_buffer<order::little, double, 64, align::no>   little_float64_buf_t;
  typedef endian_buffer<order::native, float, 32, align::no>    native_float32_buf_t;
  typedef endian_buffer<order::native, double, 64, align::no>   native_float64_buf_t;

  // Stream inserter
  template <class charT, class traits, BOOST_SCOPED_ENUM(order) Order, class T,
    std::size_t n_bits, BOOST_SCOPED_ENUM(align) A>
  std::basic_ostream<charT, traits>&
    operator<<(std::basic_ostream<charT, traits>& os,
      const endian_buffer<Order, T, n_bits, A>& x)
  {
    return os << x.value();
  }

  // Stream extractor
  template <class charT, class traits, BOOST_SCOPED_ENUM(order) Order, class T,
    std::size_t n_bits, BOOST_SCOPED_ENUM(align) A>
  std::basic_istream<charT, traits>&
    operator>>(std::basic_istream<charT, traits>& is,
      endian_buffer<Order, T, n_bits, A>& x)
  {
    T i;
    if (is >> i)
      x = i;
    return is;
  }

//----------------------------------  end synopsis  ------------------------------------//

//  endian_buffer class template specializations  --------------------------------------//

//  Specializations that represent unaligned bytes.
//  Taking an integer type as a parameter provides a nice way to pass both
//  the size and signedness of the desired integer and get the appropriate
//  corresponding integer type for the interface.

// Q: Should endian_buffer supply "value_type operator value_type() const noexcept"?
// A: No. The rationale for endian_buffers is to prevent high-cost hidden
//    conversions. If an implicit conversion operator is supplied, hidden conversions
//    can occur.

//  unaligned endian_buffer specialization

template< BOOST_SCOPED_ENUM(order) Order, class T, std::size_t n_bits >
class endian_buffer<Order, T, n_bits, align::no>
{
#ifdef BOOST_ENDIAN_NO_CTORS
public:
#endif

    BOOST_STATIC_ASSERT( (n_bits/8)*8 == n_bits );

    unsigned char value_[ n_bits / 8 ];

public:

    typedef T value_type;

#ifndef BOOST_ENDIAN_NO_CTORS

    endian_buffer() BOOST_ENDIAN_DEFAULT_CONSTRUCT

    explicit endian_buffer( T val ) BOOST_NOEXCEPT
    {
        boost::endian::endian_store<T, n_bits / 8, Order>( value_, val );
    }

#endif

    endian_buffer& operator=( T val ) BOOST_NOEXCEPT
    {
        boost::endian::endian_store<T, n_bits / 8, Order>( value_, val );
        return *this;
    }

    value_type value() const BOOST_NOEXCEPT
    {
        return boost::endian::endian_load<T, n_bits / 8, Order>( value_ );
    }

    unsigned char const * data() const BOOST_NOEXCEPT
    {
        return value_;
    }

    unsigned char * data() BOOST_NOEXCEPT
    {
        return value_;
    }
};

// aligned specializations; only n_bits == 16/32/64 supported

// aligned endian_buffer specialization

template< BOOST_SCOPED_ENUM(order) Order, class T, std::size_t n_bits >
class endian_buffer<Order, T, n_bits, align::yes>
{
private:

    BOOST_STATIC_ASSERT( (n_bits/8)*8 == n_bits );
    BOOST_STATIC_ASSERT( sizeof(T) == n_bits/8 );

    union
    {
        unsigned char value_[ n_bits / 8 ];
        T align_;
    };

public:

    typedef T value_type;

#ifndef BOOST_ENDIAN_NO_CTORS

    endian_buffer() BOOST_ENDIAN_DEFAULT_CONSTRUCT

    explicit endian_buffer( T val ) BOOST_NOEXCEPT
    {
        boost::endian::endian_store<T, n_bits / 8, Order>( value_, val );
    }

#endif

    endian_buffer& operator=( T val ) BOOST_NOEXCEPT
    {
        boost::endian::endian_store<T, n_bits / 8, Order>( value_, val );
        return *this;
    }

    value_type value() const BOOST_NOEXCEPT
    {
        return boost::endian::endian_load<T, n_bits / 8, Order>( value_ );
    }

    unsigned char const * data() const BOOST_NOEXCEPT
    {
        return value_;
    }

    unsigned char * data() BOOST_NOEXCEPT
    {
        return value_;
    }
};

// aligned native endian_buffer specialization

template< class T, std::size_t n_bits >
class endian_buffer<order::native, T, n_bits, align::yes>
{
private:

    BOOST_STATIC_ASSERT( (n_bits/8)*8 == n_bits );
    BOOST_STATIC_ASSERT( sizeof(T) == n_bits/8 );

    T value_;

public:

    typedef T value_type;

#ifndef BOOST_ENDIAN_NO_CTORS

    endian_buffer() BOOST_ENDIAN_DEFAULT_CONSTRUCT

    explicit endian_buffer( T val ) BOOST_NOEXCEPT: value_( val )
    {
    }

#endif

    endian_buffer& operator=( T val ) BOOST_NOEXCEPT
    {
        value_ = val;
        return *this;
    }

    value_type value() const BOOST_NOEXCEPT
    {
        return value_;
    }

    unsigned char const * data() const BOOST_NOEXCEPT
    {
        return reinterpret_cast< unsigned char const* >( &value_ );
    }

    unsigned char * data() BOOST_NOEXCEPT
    {
        return reinterpret_cast< unsigned char* >( &value_ );
    }
};

} // namespace endian
} // namespace boost

#if defined(BOOST_BORLANDC) || defined(BOOST_CODEGEARC)
# pragma pack(pop)
#endif

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif // BOOST_ENDIAN_BUFFERS_HPP
