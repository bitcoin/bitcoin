//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_TCP_STREAM_HPP
#define BOOST_BEAST_CORE_TCP_STREAM_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/basic_stream.hpp>
#include <boost/beast/core/rate_policy.hpp>
#include <boost/asio/executor.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace boost {
namespace beast {

/** A TCP/IP stream socket with timeouts and a polymorphic executor.

    @see basic_stream
*/
using tcp_stream = basic_stream<
    net::ip::tcp,
    net::any_io_executor,
    unlimited_rate_policy>;

} // beast
} // boost

#endif
