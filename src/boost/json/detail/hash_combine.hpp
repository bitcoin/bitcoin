//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_HASH_COMBINE_HPP
#define BOOST_JSON_DETAIL_HASH_COMBINE_HPP

BOOST_JSON_NS_BEGIN
namespace detail {

inline
std::size_t
hash_combine(
    std::size_t seed,
    std::size_t h) noexcept
{
  seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
  return seed;
}

inline
std::size_t
hash_combine_commutative(
    std::size_t lhs,
    std::size_t rhs) noexcept
{
  return  lhs ^ rhs;
}

} // detail
BOOST_JSON_NS_END

#endif
