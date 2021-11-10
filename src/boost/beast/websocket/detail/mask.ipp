//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_MASK_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_MASK_IPP

#include <boost/beast/websocket/detail/mask.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

void
prepare_key(prepared_key& prepared, std::uint32_t key)
{
    prepared[0] = (key >>  0) & 0xff;
    prepared[1] = (key >>  8) & 0xff;
    prepared[2] = (key >> 16) & 0xff;
    prepared[3] = (key >> 24) & 0xff;
}

inline
void
rol(prepared_key& v, std::size_t n)
{
    auto v0 = v;
    for(std::size_t i = 0; i < v.size(); ++i )
        v[i] = v0[(i + n) % v.size()];
}

// Apply mask in place
//
void
mask_inplace(net::mutable_buffer const& b, prepared_key& key)
{
    auto n = b.size();
    auto const mask = key; // avoid aliasing
    auto p = static_cast<unsigned char*>(b.data());
    while(n >= 4)
    {
        for(int i = 0; i < 4; ++i)
            p[i] ^= mask[i];
        p += 4;
        n -= 4;
    }
    if(n > 0)
    {
        for(std::size_t i = 0; i < n; ++i)
            p[i] ^= mask[i];
        rol(key, n);
    }
}

} // detail
} // websocket
} // beast
} // boost

#endif
