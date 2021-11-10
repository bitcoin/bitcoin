//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_RFC6455_HPP
#define BOOST_BEAST_WEBSOCKET_IMPL_RFC6455_HPP

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/rfc7230.hpp>

namespace boost {
namespace beast {
namespace websocket {

template<class Allocator>
bool
is_upgrade(http::header<true,
    http::basic_fields<Allocator>> const& req)
{
    if(req.version() < 11)
        return false;
    if(req.method() != http::verb::get)
        return false;
    if(! http::token_list{req[http::field::connection]}.exists("upgrade"))
        return false;
    if(! http::token_list{req[http::field::upgrade]}.exists("websocket"))
        return false;
    return true;
}

} // websocket
} // beast
} // boost

#endif
