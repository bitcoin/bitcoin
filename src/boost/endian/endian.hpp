//  boost/endian/endian.hpp  -----------------------------------------------------------//

//  Copyright Beman Dawes 2015

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See library home page at http://www.boost.org/libs/endian

#ifndef BOOST_ENDIAN_ENDIAN_HPP
#define BOOST_ENDIAN_ENDIAN_HPP

#ifndef BOOST_ENDIAN_DEPRECATED_NAMES
# error "<boost/endian/endian.hpp> is deprecated. Define BOOST_ENDIAN_DEPRECATED_NAMES to use."
#endif

#include <boost/config/header_deprecated.hpp>

BOOST_HEADER_DEPRECATED( "<boost/endian/arithmetic.hpp>" )

#include <boost/endian/arithmetic.hpp>
#include <boost/config.hpp>

namespace boost
{
namespace endian
{
  typedef order endianness;
  typedef align alignment;

# ifndef  BOOST_NO_CXX11_TEMPLATE_ALIASES
  template <BOOST_SCOPED_ENUM(order) Order, class T, std::size_t n_bits,
    BOOST_SCOPED_ENUM(align) Align = align::no>
  using endian = endian_arithmetic<Order, T, n_bits, Align>;
# endif

  // unaligned big endian signed integer types
  typedef endian_arithmetic< order::big, int_least8_t, 8 >           big8_t;
  typedef endian_arithmetic< order::big, int_least16_t, 16 >         big16_t;
  typedef endian_arithmetic< order::big, int_least32_t, 24 >         big24_t;
  typedef endian_arithmetic< order::big, int_least32_t, 32 >         big32_t;
  typedef endian_arithmetic< order::big, int_least64_t, 40 >         big40_t;
  typedef endian_arithmetic< order::big, int_least64_t, 48 >         big48_t;
  typedef endian_arithmetic< order::big, int_least64_t, 56 >         big56_t;
  typedef endian_arithmetic< order::big, int_least64_t, 64 >         big64_t;

  // unaligned big endian_arithmetic unsigned integer types
  typedef endian_arithmetic< order::big, uint_least8_t, 8 >          ubig8_t;
  typedef endian_arithmetic< order::big, uint_least16_t, 16 >        ubig16_t;
  typedef endian_arithmetic< order::big, uint_least32_t, 24 >        ubig24_t;
  typedef endian_arithmetic< order::big, uint_least32_t, 32 >        ubig32_t;
  typedef endian_arithmetic< order::big, uint_least64_t, 40 >        ubig40_t;
  typedef endian_arithmetic< order::big, uint_least64_t, 48 >        ubig48_t;
  typedef endian_arithmetic< order::big, uint_least64_t, 56 >        ubig56_t;
  typedef endian_arithmetic< order::big, uint_least64_t, 64 >        ubig64_t;

  // unaligned little endian_arithmetic signed integer types
  typedef endian_arithmetic< order::little, int_least8_t, 8 >        little8_t;
  typedef endian_arithmetic< order::little, int_least16_t, 16 >      little16_t;
  typedef endian_arithmetic< order::little, int_least32_t, 24 >      little24_t;
  typedef endian_arithmetic< order::little, int_least32_t, 32 >      little32_t;
  typedef endian_arithmetic< order::little, int_least64_t, 40 >      little40_t;
  typedef endian_arithmetic< order::little, int_least64_t, 48 >      little48_t;
  typedef endian_arithmetic< order::little, int_least64_t, 56 >      little56_t;
  typedef endian_arithmetic< order::little, int_least64_t, 64 >      little64_t;

  // unaligned little endian_arithmetic unsigned integer types
  typedef endian_arithmetic< order::little, uint_least8_t, 8 >       ulittle8_t;
  typedef endian_arithmetic< order::little, uint_least16_t, 16 >     ulittle16_t;
  typedef endian_arithmetic< order::little, uint_least32_t, 24 >     ulittle24_t;
  typedef endian_arithmetic< order::little, uint_least32_t, 32 >     ulittle32_t;
  typedef endian_arithmetic< order::little, uint_least64_t, 40 >     ulittle40_t;
  typedef endian_arithmetic< order::little, uint_least64_t, 48 >     ulittle48_t;
  typedef endian_arithmetic< order::little, uint_least64_t, 56 >     ulittle56_t;
  typedef endian_arithmetic< order::little, uint_least64_t, 64 >     ulittle64_t;

  // unaligned native endian_arithmetic signed integer types
  typedef endian_arithmetic< order::native, int_least8_t, 8 >        native8_t;
  typedef endian_arithmetic< order::native, int_least16_t, 16 >      native16_t;
  typedef endian_arithmetic< order::native, int_least32_t, 24 >      native24_t;
  typedef endian_arithmetic< order::native, int_least32_t, 32 >      native32_t;
  typedef endian_arithmetic< order::native, int_least64_t, 40 >      native40_t;
  typedef endian_arithmetic< order::native, int_least64_t, 48 >      native48_t;
  typedef endian_arithmetic< order::native, int_least64_t, 56 >      native56_t;
  typedef endian_arithmetic< order::native, int_least64_t, 64 >      native64_t;

  // unaligned native endian_arithmetic unsigned integer types
  typedef endian_arithmetic< order::native, uint_least8_t, 8 >       unative8_t;
  typedef endian_arithmetic< order::native, uint_least16_t, 16 >     unative16_t;
  typedef endian_arithmetic< order::native, uint_least32_t, 24 >     unative24_t;
  typedef endian_arithmetic< order::native, uint_least32_t, 32 >     unative32_t;
  typedef endian_arithmetic< order::native, uint_least64_t, 40 >     unative40_t;
  typedef endian_arithmetic< order::native, uint_least64_t, 48 >     unative48_t;
  typedef endian_arithmetic< order::native, uint_least64_t, 56 >     unative56_t;
  typedef endian_arithmetic< order::native, uint_least64_t, 64 >     unative64_t;

  // aligned native endian_arithmetic typedefs are not provided because
  // <cstdint> types are superior for this use case

  typedef endian_arithmetic< order::big, int16_t, 16, align::yes >      aligned_big16_t;
  typedef endian_arithmetic< order::big, uint16_t, 16, align::yes >     aligned_ubig16_t;
  typedef endian_arithmetic< order::little, int16_t, 16, align::yes >   aligned_little16_t;
  typedef endian_arithmetic< order::little, uint16_t, 16, align::yes >  aligned_ulittle16_t;

  typedef endian_arithmetic< order::big, int32_t, 32, align::yes >      aligned_big32_t;
  typedef endian_arithmetic< order::big, uint32_t, 32, align::yes >     aligned_ubig32_t;
  typedef endian_arithmetic< order::little, int32_t, 32, align::yes >   aligned_little32_t;
  typedef endian_arithmetic< order::little, uint32_t, 32, align::yes >  aligned_ulittle32_t;

  typedef endian_arithmetic< order::big, int64_t, 64, align::yes >      aligned_big64_t;
  typedef endian_arithmetic< order::big, uint64_t, 64, align::yes >     aligned_ubig64_t;
  typedef endian_arithmetic< order::little, int64_t, 64, align::yes >   aligned_little64_t;
  typedef endian_arithmetic< order::little, uint64_t, 64, align::yes >  aligned_ulittle64_t;

} // namespace endian
} // namespace boost

#endif  //BOOST_ENDIAN_ENDIAN_HPP
