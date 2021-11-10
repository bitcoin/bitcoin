//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_HYBI13_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_HYBI13_IPP

#include <boost/beast/websocket/detail/hybi13.hpp>
#include <boost/beast/core/detail/sha1.hpp>
#include <boost/beast/websocket/detail/prng.hpp>

#include <boost/assert.hpp>
#include <cstdint>
#include <string>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

void
make_sec_ws_key(sec_ws_key_type& key)
{
    auto g = make_prng(true);
    std::uint32_t a[4];
    for (auto& v : a)
        v = g();
    key.resize(key.max_size());
    key.resize(beast::detail::base64::encode(
        key.data(), &a[0], sizeof(a)));
}

void
make_sec_ws_accept(
    sec_ws_accept_type& accept,
    string_view key)
{
    BOOST_ASSERT(key.size() <= sec_ws_key_type::max_size_n);
    using namespace beast::detail::string_literals;
    auto const guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"_sv;
    beast::detail::sha1_context ctx;
    beast::detail::init(ctx);
    beast::detail::update(ctx, key.data(), key.size());
    beast::detail::update(ctx, guid.data(), guid.size());
    char digest[beast::detail::sha1_context::digest_size];
    beast::detail::finish(ctx, &digest[0]);
    accept.resize(accept.max_size());
    accept.resize(beast::detail::base64::encode(
        accept.data(), &digest[0], sizeof(digest)));
}

} // detail
} // websocket
} // beast
} // boost

#endif // BOOST_BEAST_WEBSOCKET_DETAIL_HYBI13_IPP
