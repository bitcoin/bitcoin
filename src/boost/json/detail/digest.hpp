//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/json
//

#ifndef BOOST_JSON_DETAIL_DIGEST_HPP
#define BOOST_JSON_DETAIL_DIGEST_HPP

BOOST_JSON_NS_BEGIN
namespace detail {

// Calculate salted digest of string
inline
std::size_t
digest(
    char const* s,
    std::size_t n,
    std::size_t salt) noexcept
{
#if BOOST_JSON_ARCH == 64
    std::uint64_t const prime = 0x100000001B3ULL;
    std::uint64_t hash  = 0xcbf29ce484222325ULL;
#else
    std::uint32_t const prime = 0x01000193UL;
    std::uint32_t hash  = 0x811C9DC5UL;
#endif
    hash += salt;
    for(;n--;++s)
        hash = (*s ^ hash) * prime;
    return hash;
}

} // detail
BOOST_JSON_NS_END

#endif
