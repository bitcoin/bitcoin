//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_SHA1_HPP
#define BOOST_BEAST_DETAIL_SHA1_HPP

#include <boost/beast/core/detail/config.hpp>
#include <cstdint>
#include <cstddef>

// Based on https://github.com/vog/sha1
/*
    Original authors:
        Steve Reid (Original C Code)
        Bruce Guenter (Small changes to fit into bglibs)
        Volker Grabsch (Translation to simpler C++ Code)
        Eugene Hopkinson (Safety improvements)
        Vincent Falco (beast adaptation)
*/

namespace boost {
namespace beast {
namespace detail {

namespace sha1 {

static std::size_t constexpr BLOCK_INTS = 16;
static std::size_t constexpr BLOCK_BYTES = 64;
static std::size_t constexpr DIGEST_BYTES = 20;

} // sha1

struct sha1_context
{
    static unsigned int constexpr block_size = sha1::BLOCK_BYTES;
    static unsigned int constexpr digest_size = 20;

    std::size_t buflen;
    std::size_t blocks;
    std::uint32_t digest[5];
    std::uint8_t buf[block_size];
};

BOOST_BEAST_DECL
void
init(sha1_context& ctx) noexcept;

BOOST_BEAST_DECL
void
update(
    sha1_context& ctx,
    void const* message,
    std::size_t size) noexcept;

BOOST_BEAST_DECL
void
finish(
    sha1_context& ctx,
    void* digest) noexcept;

} // detail
} // beast
} // boost

#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/core/detail/sha1.ipp>
#endif

#endif
