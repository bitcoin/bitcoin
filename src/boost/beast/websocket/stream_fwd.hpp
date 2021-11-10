//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_STREAM_FWD_HPP
#define BOOST_BEAST_WEBSOCKET_STREAM_FWD_HPP

#include <boost/beast/core/detail/config.hpp>

//[code_websocket_1h

namespace boost {
namespace beast {
namespace websocket {

template<
    class NextLayer,
    bool deflateSupported = true>
class stream;

} // websocket
} // beast
} // boost

//]

#endif
