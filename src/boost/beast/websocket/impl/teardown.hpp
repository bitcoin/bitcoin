//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_TEARDOWN_HPP
#define BOOST_BEAST_WEBSOCKET_IMPL_TEARDOWN_HPP

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/bind_continuation.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/post.hpp>
#include <memory>

namespace boost {
namespace beast {
namespace websocket {

namespace detail {

template<
    class Protocol, class Executor,
    class Handler>
class teardown_tcp_op
    : public beast::async_base<
        Handler, beast::executor_type<
            net::basic_stream_socket<
                Protocol, Executor>>>
    , public asio::coroutine
{
    using socket_type =
        net::basic_stream_socket<Protocol, Executor>;

    socket_type& s_;
    role_type role_;
    bool nb_;

public:
    template<class Handler_>
    teardown_tcp_op(
        Handler_&& h,
        socket_type& s,
        role_type role)
        : async_base<Handler,
            beast::executor_type<
                net::basic_stream_socket<
                    Protocol, Executor>>>(
            std::forward<Handler_>(h),
            s.get_executor())
        , s_(s)
        , role_(role)
        , nb_(false)
    {
        (*this)({}, 0, false);
    }

    void
    operator()(
        error_code ec = {},
        std::size_t bytes_transferred = 0,
        bool cont = true)
    {
        BOOST_ASIO_CORO_REENTER(*this)
        {
            nb_ = s_.non_blocking();
            s_.non_blocking(true, ec);
            if(ec)
                goto upcall;
            if(role_ == role_type::server)
                s_.shutdown(net::socket_base::shutdown_send, ec);
            if(ec)
                goto upcall;
            for(;;)
            {
                {
                    char buf[2048];
                    s_.read_some(net::buffer(buf), ec);
                }
                if(ec == net::error::would_block)
                {
                    BOOST_ASIO_CORO_YIELD
                    {
                        BOOST_ASIO_HANDLER_LOCATION((
                            __FILE__, __LINE__,
                            "websocket::tcp::async_teardown"
                        ));

                        s_.async_wait(
                            net::socket_base::wait_read,
                                beast::detail::bind_continuation(std::move(*this)));
                    }
                    continue;
                }
                if(ec)
                {
                    if(ec != net::error::eof)
                        goto upcall;
                    ec = {};
                    break;
                }
                if(bytes_transferred == 0)
                {
                    // happens sometimes
                    // https://github.com/boostorg/beast/issues/1373
                    break;
                }
            }
            if(role_ == role_type::client)
                s_.shutdown(net::socket_base::shutdown_send, ec);
            if(ec)
                goto upcall;
            s_.close(ec);
        upcall:
            if(! cont)
            {
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "websocket::tcp::async_teardown"
                        ));

                    net::post(bind_front_handler(
                        std::move(*this), ec));
                }
            }
            {
                error_code ignored;
                s_.non_blocking(nb_, ignored);
            }
            this->complete_now(ec);
        }
    }
};

} // detail

//------------------------------------------------------------------------------

template<class Protocol, class Executor>
void
teardown(
    role_type role,
    net::basic_stream_socket<
        Protocol, Executor>& socket,
    error_code& ec)
{
    if(role == role_type::server)
        socket.shutdown(
            net::socket_base::shutdown_send, ec);
    if(ec)
        return;
    for(;;)
    {
        char buf[2048];
        auto const bytes_transferred =
            socket.read_some(net::buffer(buf), ec);
        if(ec)
        {
            if(ec != net::error::eof)
                return;
            ec = {};
            break;
        }
        if(bytes_transferred == 0)
        {
            // happens sometimes
            // https://github.com/boostorg/beast/issues/1373
            break;
        }
    }
    if(role == role_type::client)
        socket.shutdown(
            net::socket_base::shutdown_send, ec);
    if(ec)
        return;
    socket.close(ec);
}

template<
    class Protocol, class Executor,
    class TeardownHandler>
void
async_teardown(
    role_type role,
    net::basic_stream_socket<
        Protocol, Executor>& socket,
    TeardownHandler&& handler)
{
    static_assert(beast::detail::is_invocable<
        TeardownHandler, void(error_code)>::value,
            "TeardownHandler type requirements not met");
    detail::teardown_tcp_op<
        Protocol,
        Executor,
        typename std::decay<TeardownHandler>::type>(
            std::forward<TeardownHandler>(handler),
            socket,
            role);
}

} // websocket
} // beast
} // boost

#endif
