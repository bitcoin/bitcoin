//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_ICY_STREAM_HPP
#define BOOST_BEAST_CORE_IMPL_ICY_STREAM_HPP

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <cstring>
#include <memory>
#include <utility>

namespace boost {
namespace beast {
namespace http {

namespace detail {

template<class ConstBufferSequence>
boost::tribool
is_icy(ConstBufferSequence const& buffers)
{
    char buf[3];
    auto const n = net::buffer_copy(
        net::mutable_buffer(buf, 3),
        buffers);
    if(n >= 1 && buf[0] != 'I')
        return false;
    if(n >= 2 && buf[1] != 'C')
        return false;
    if(n >= 3 && buf[2] != 'Y')
        return false;
    if(n < 3)
        return boost::indeterminate;
    return true;
}

} // detail

template<class NextLayer>
struct icy_stream<NextLayer>::ops
{

template<class Buffers, class Handler>
class read_op
    : public beast::async_base<Handler,
        beast::executor_type<icy_stream>>
    , public asio::coroutine
{
    icy_stream& s_;
    Buffers b_;
    std::size_t n_ = 0;
    error_code ec_;
    bool match_ = false;

public:
    template<class Handler_>
    read_op(
        Handler_&& h,
        icy_stream& s,
        Buffers const& b)
        : async_base<Handler,
            beast::executor_type<icy_stream>>(
                std::forward<Handler_>(h), s.get_executor())
        , s_(s)
        , b_(b)
    {
        (*this)({}, 0, false);
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred,
        bool cont = true)
    {
        BOOST_ASIO_CORO_REENTER(*this)
        {
            if(s_.detect_)
            {
                BOOST_ASSERT(s_.n_ == 0);
                for(;;)
                {
                    // Try to read the first three characters
                    BOOST_ASIO_CORO_YIELD
                    {
                        BOOST_ASIO_HANDLER_LOCATION((
                            __FILE__, __LINE__,
                            "http::icy_stream::async_read_some"));

                        s_.next_layer().async_read_some(
                            net::mutable_buffer(
                                s_.buf_ + s_.n_, 3 - s_.n_),
                            std::move(*this));
                    }
                    s_.n_ += static_cast<char>(bytes_transferred);
                    if(ec)
                        goto upcall;
                    auto result = detail::is_icy(
                        net::const_buffer(s_.buf_, s_.n_));
                    if(boost::indeterminate(result))
                        continue;
                    if(result)
                        s_.n_ = static_cast<char>(net::buffer_copy(
                            net::buffer(s_.buf_, sizeof(s_.buf_)),
                            icy_stream::version()));
                    break;
                }
                s_.detect_ = false;
            }
            if(s_.n_ > 0)
            {
                bytes_transferred = net::buffer_copy(
                    b_, net::const_buffer(s_.buf_, s_.n_));
                s_.n_ -= static_cast<char>(bytes_transferred);
                std::memmove(
                    s_.buf_,
                    s_.buf_ + bytes_transferred,
                    sizeof(s_.buf_) - bytes_transferred);
            }
            else
            {
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "http::icy_stream::async_read_some"));

                    s_.next_layer().async_read_some(
                        b_, std::move(*this));
                }
            }
        upcall:
            if(! cont)
            {
                ec_ = ec;
                n_ = bytes_transferred;
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "http::icy_stream::async_read_some"));

                    s_.next_layer().async_read_some(
                        net::mutable_buffer{},
                        std::move(*this));
                }
                ec = ec_;
                bytes_transferred = n_;
            }
            this->complete_now(ec, bytes_transferred);
        }
    }
};

struct run_read_op
{
    template<class ReadHandler, class Buffers>
    void
    operator()(
        ReadHandler&& h,
        icy_stream* s,
        Buffers const& b)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<ReadHandler,
            void(error_code, std::size_t)>::value,
            "ReadHandler type requirements not met");

        read_op<
            Buffers,
            typename std::decay<ReadHandler>::type>(
                std::forward<ReadHandler>(h), *s, b);
    }
};

};

//------------------------------------------------------------------------------

template<class NextLayer>
template<class... Args>
icy_stream<NextLayer>::
icy_stream(Args&&... args)
    : stream_(std::forward<Args>(args)...)
{
    std::memset(buf_, 0, sizeof(buf_));
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
icy_stream<NextLayer>::
read_some(MutableBufferSequence const& buffers)
{
    static_assert(is_sync_read_stream<next_layer_type>::value,
        "SyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    error_code ec;
    auto n = read_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return n;
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
icy_stream<NextLayer>::
read_some(MutableBufferSequence const& buffers, error_code& ec)
{
    static_assert(is_sync_read_stream<next_layer_type>::value,
        "SyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    std::size_t bytes_transferred;
    if(detect_)
    {
        BOOST_ASSERT(n_ == 0);
        for(;;)
        {
            // Try to read the first three characters
            bytes_transferred = next_layer().read_some(
                net::mutable_buffer(buf_ + n_, 3 - n_), ec);
            n_ += static_cast<char>(bytes_transferred);
            if(ec)
                return 0;
            auto result = detail::is_icy(
                net::const_buffer(buf_, n_));
            if(boost::indeterminate(result))
                continue;
            if(result)
                n_ = static_cast<char>(net::buffer_copy(
                    net::buffer(buf_, sizeof(buf_)),
                    icy_stream::version()));
            break;
        }
        detect_ = false;
    }
    if(n_ > 0)
    {
        bytes_transferred = net::buffer_copy(
            buffers, net::const_buffer(buf_, n_));
        n_ -= static_cast<char>(bytes_transferred);
        std::memmove(
            buf_,
            buf_ + bytes_transferred,
            sizeof(buf_) - bytes_transferred);
    }
    else
    {
        bytes_transferred =
            next_layer().read_some(buffers, ec);
    }
    return bytes_transferred;
}

template<class NextLayer>
template<
    class MutableBufferSequence,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
icy_stream<NextLayer>::
async_read_some(
    MutableBufferSequence const& buffers,
    ReadHandler&& handler)
{
    static_assert(is_async_read_stream<next_layer_type>::value,
        "AsyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
            MutableBufferSequence >::value,
        "MutableBufferSequence type requirements not met");
    return net::async_initiate<
        ReadHandler,
        void(error_code, std::size_t)>(
            typename ops::run_read_op{},
            handler,
            this,
            buffers);
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
icy_stream<NextLayer>::
write_some(MutableBufferSequence const& buffers)
{
    static_assert(is_sync_write_stream<next_layer_type>::value,
        "SyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    return stream_.write_some(buffers);
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
icy_stream<NextLayer>::
write_some(MutableBufferSequence const& buffers, error_code& ec)
{
    static_assert(is_sync_write_stream<next_layer_type>::value,
        "SyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    return stream_.write_some(buffers, ec);
}

template<class NextLayer>
template<
    class MutableBufferSequence,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
icy_stream<NextLayer>::
async_write_some(
    MutableBufferSequence const& buffers,
    WriteHandler&& handler)
{
    static_assert(is_async_write_stream<next_layer_type>::value,
        "AsyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");
    return stream_.async_write_some(
        buffers, std::forward<WriteHandler>(handler));
}

} // http
} // beast
} // boost

#endif
