//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_ROLE_HPP
#define BOOST_BEAST_ROLE_HPP

#include <boost/beast/core/detail/config.hpp>

namespace boost {
namespace beast {

/** The role of local or remote peer.

    Whether the endpoint is a client or server affects the
    behavior of teardown.
    The teardown behavior also depends on the type of the stream
    being torn down.
    
    The default implementation of teardown for regular
    TCP/IP sockets is as follows:

    @li In the client role, a TCP/IP shutdown is sent after
    reading all remaining data on the connection.

    @li In the server role, a TCP/IP shutdown is sent before
    reading all remaining data on the connection.

    When the next layer type is a `net::ssl::stream`,
    the connection is closed by performing the SSL closing
    handshake corresponding to the role type, client or server.
*/
enum class role_type
{
    /// The stream is operating as a client.
    client,

    /// The stream is operating as a server.
    server
};

} // beast
} // boost

#endif
