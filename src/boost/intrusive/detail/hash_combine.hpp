// Copyright 2005-2014 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  Based on Peter Dimov's proposal
//  http://www.open-std.org/JTC1/SC22/WG21/docs/papers/2005/n1756.pdf
//  issue 6.18.
//
//  This also contains public domain code from MurmurHash. From the
//  MurmurHash header:
//
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Copyright 2021 Ion Gaztanaga
// Refactored the original boost/container_hash/hash.hpp to avoid
// any heavy std header dependencies to just combine two hash
// values represented in a std::size_t type.

#ifndef BOOST_INTRUSIVE_DETAIL_HASH_COMBINE_HPP
#define BOOST_INTRUSIVE_DETAIL_HASH_COMBINE_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif

#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/cstdint.hpp>

#if defined(_MSC_VER)
#  include <stdlib.h>
#  define BOOST_INTRUSIVE_HASH_ROTL32(x, r) _rotl(x,r)
#else
#  define BOOST_INTRUSIVE_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

namespace boost {
namespace intrusive {
namespace detail {

template <typename SizeT>
inline void hash_combine_size_t(SizeT& seed, SizeT value)
{
   seed ^= value + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

inline void hash_combine_size_t(boost::uint32_t& h1, boost::uint32_t k1)
{
   const uint32_t c1 = 0xcc9e2d51;
   const uint32_t c2 = 0x1b873593;

   k1 *= c1;
   k1 = BOOST_INTRUSIVE_HASH_ROTL32(k1,15);
   k1 *= c2;

   h1 ^= k1;
   h1 = BOOST_INTRUSIVE_HASH_ROTL32(h1,13);
   h1 = h1*5+0xe6546b64;
}


   // Don't define 64-bit hash combine on platforms without 64 bit integers,
   // and also not for 32-bit gcc as it warns about the 64-bit constant.
   #if !defined(BOOST_NO_INT64_T) && \
       !(defined(__GNUC__) && ULONG_MAX == 0xffffffff)
   inline void hash_combine_size_t(boost::uint64_t& h, boost::uint64_t k)
   {
      const boost::uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
      const int r = 47;

      k *= m;
      k ^= k >> r;
      k *= m;

      h ^= k;
      h *= m;

      // Completely arbitrary number, to prevent 0's
      // from hashing to 0.
      h += 0xe6546b64;
   }

   #endif // BOOST_NO_INT64_T

}  //namespace detail {
}  //namespace intrusive {
}  //namespace boost {

#endif   //BOOST_INTRUSIVE_DETAIL_HASH_COMBINE_HPP
