//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_TYPE_TRAITS_HPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_TYPE_TRAITS_HPP

#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

template<class F>
using is_request_decorator =
    typename beast::detail::is_invocable<F,
        void(request_type&)>::type;

template<class F>
using is_response_decorator =
    typename beast::detail::is_invocable<F,
        void(response_type&)>::type;

} // detail
} // websocket
} // beast
} // boost

#endif
