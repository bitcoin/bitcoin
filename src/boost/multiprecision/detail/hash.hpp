///////////////////////////////////////////////////////////////////////////////
//  Copyright 2021 Matt Borland. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MULTIPRECISION_DETAIL_HASH_HPP
#define BOOST_MULTIPRECISION_DETAIL_HASH_HPP

#include <cstddef>
#include <functional>

namespace boost { namespace multiprecision { namespace detail {

template <typename T>
inline std::size_t hash_value(const T& v)
{
    std::hash<T> hasher;
    return hasher(v);
}

#if defined(BOOST_HAS_INT128)

std::size_t hash_value(const unsigned __int128& val);

inline std::size_t hash_value(const __int128& val)
{
   return hash_value(static_cast<unsigned __int128>(val));
}

#endif

inline void hash_combine(std::size_t&) {}

template <typename T, typename... Args>
inline void hash_combine(std::size_t& seed, const T& v, Args... args) 
{
    constexpr std::size_t adder = 0x9e3779b9;
    seed = seed ^ (hash_value(v) + adder + (seed<<6) + (seed>>2));
    hash_combine(seed, args...);
}

#if defined(BOOST_HAS_INT128)

inline std::size_t hash_value(const unsigned __int128& val)
{
   std::size_t result = static_cast<std::size_t>(val);
   hash_combine(result, static_cast<std::size_t>(val >> 64));
   return result;
}

#endif

}}} // Namespaces

#endif // BOOST_MULTIPRECISION_DETAIL_HASH_HPP
