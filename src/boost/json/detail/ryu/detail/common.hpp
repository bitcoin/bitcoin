// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

/*
    This is a derivative work
*/

#ifndef BOOST_JSON_DETAIL_RYU_DETAIL_COMMON_HPP
#define BOOST_JSON_DETAIL_RYU_DETAIL_COMMON_HPP

#include <boost/json/detail/config.hpp>
#include <string.h>

BOOST_JSON_NS_BEGIN
namespace detail {

namespace ryu {
namespace detail {

constexpr int DOUBLE_MANTISSA_BITS = 52;
constexpr int DOUBLE_EXPONENT_BITS = 11;
constexpr int DOUBLE_BIAS = 1023;

#if defined(_M_IX86) || defined(_M_ARM)
#define BOOST_JSON_RYU_32_BIT_PLATFORM
#endif

inline uint32_t decimalLength9(const uint32_t v) {
  // Function precondition: v is not a 10-digit number.
  // (f2s: 9 digits are sufficient for round-tripping.)
  // (d2fixed: We print 9-digit blocks.)
  BOOST_ASSERT(v < 1000000000);
  if (v >= 100000000) { return 9; }
  if (v >= 10000000) { return 8; }
  if (v >= 1000000) { return 7; }
  if (v >= 100000) { return 6; }
  if (v >= 10000) { return 5; }
  if (v >= 1000) { return 4; }
  if (v >= 100) { return 3; }
  if (v >= 10) { return 2; }
  return 1;
}

// Returns e == 0 ? 1 : ceil(log_2(5^e)).
inline int32_t pow5bits(const int32_t e) {
  // This approximation works up to the point that the multiplication overflows at e = 3529.
  // If the multiplication were done in 64 bits, it would fail at 5^4004 which is just greater
  // than 2^9297.
  BOOST_ASSERT(e >= 0);
  BOOST_ASSERT(e <= 3528);
  return (int32_t) (((((uint32_t) e) * 1217359) >> 19) + 1);
}

// Returns floor(log_10(2^e)).
inline uint32_t log10Pow2(const int32_t e) {
  // The first value this approximation fails for is 2^1651 which is just greater than 10^297.
  BOOST_ASSERT(e >= 0);
  BOOST_ASSERT(e <= 1650);
  return (((uint32_t) e) * 78913) >> 18;
}

// Returns floor(log_10(5^e)).
inline uint32_t log10Pow5(const int32_t e) {
  // The first value this approximation fails for is 5^2621 which is just greater than 10^1832.
  BOOST_ASSERT(e >= 0);
  BOOST_ASSERT(e <= 2620);
  return (((uint32_t) e) * 732923) >> 20;
}

inline int copy_special_str(char * const result, const bool sign, const bool exponent, const bool mantissa) {
  if (mantissa) {
    memcpy(result, "NaN", 3);
    return 3;
  }
  if (sign) {
    result[0] = '-';
  }
  if (exponent) {
    memcpy(result + sign, "Infinity", 8);
    return sign + 8;
  }
  memcpy(result + sign, "0E0", 3);
  return sign + 3;
}

inline uint32_t float_to_bits(const float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, sizeof(float));
  return bits;
}

inline uint64_t double_to_bits(const double d) {
  uint64_t bits = 0;
  memcpy(&bits, &d, sizeof(double));
  return bits;
}

} // detail
} // ryu

} // detail
BOOST_JSON_NS_END

#endif
