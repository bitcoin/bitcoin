//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_IMPL_BUFFERED_READ_STREAM_HPP
#define BOOST_BEAST_IMPL_BUFFERED_READ_STREAM_HPP

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/read_size.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/asio/post.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
namespace beast {


template<class Stream, class DynamicBuffer>
struct buffered_read_stream<Stream, DynamicBuffer>::ops
{

template<class MutableBufferSequence, class Handler>
class read_op
    : public async_base<Handler,
    beast::executor_type<buffered_read_stream>>
{
    buffered_read_stream& s_;
    MutableBufferSequence b_;
    int step_ = 0;

public:
    read_op(read_op&&) = default;
    read_op(read_op const&) = delete;

    template<class Handler_>
    read_op(
        Handler_&& h,
        buffered_read_stream& s,
        MutableBufferSequence const& b)
        : async_base<
            Handler, beast::executor_type<buffered_read_stream>>(
                std::forward<Handler_>(h), s.get_executor())
        , s_(s)
        , b_(b)
    {
        (*this)({}, 0);
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred)
    {
        // VFALCO TODO Rewrite this using reenter/yield
        switch(step_)
        {
        case 0:
            if(s_.buffer_.size() == 0)
            {
                if(s_.capacity_ == 0)
                {
                    // read (unbuffered)
                    step_ = 1;
                    return s_.next_layer_.async_read_some(
                        b_, std::move(*this));
                }
                // read
                step_ = 2;
                return s_.next_layer_.async_read_some(
                    s_.buffer_.prepare(read_size(
                        s_.buffer_, s_.capacity_)),
                            std::move(*this));
            }
            step_ = 3;
            return net::post(
                s_.get_executor(),
                beast::bind_front_handler(
                    std::move(*this), ec, 0));

        case 1:
            // upcall
            break;

        case 2:
            s_.buffer_.commit(bytes_transferred);
            BOOST_FALLTHROUGH;

        case 3:
            bytes_transferred =
                net::buffer_copy(b_, s_.buffer_.data());
            s_.buffer_.consume(bytes_transferred);
            break;
        }
        this->complete_now(ec, bytes_transferred);
    }
};

struct run_read_op
{
    template<class ReadHandler, class Buffers>
    void
    operator()(
        ReadHandler&& h,
        buffered_read_stream* s,
        Buffers const* b)
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
                std::forward<ReadHandler>(h), *s, *b);
    }
};

};

//------------------------------------------------------------------------------

template<class Stream, class DynamicBuffer>
template<class... Args>
buffered_read_stream<Stream, DynamicBuffer>::
buffered_read_stream(Args&&... args)
    : next_layer_(std::forward<Args>(args)...)
{
}

template<class Stream, class DynamicBuffer>
template<class ConstBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
buffered_read_stream<Stream, DynamicBuffer>::
async_write_some(
    ConstBufferSequence const& buffers,
    WriteHandler&& handler)
{
    static_assert(is_async_write_stream<next_layer_type>::value,
        "AsyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    static_assert(detail::is_completion_token_for<WriteHandler,
        void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");
    return next_layer_.async_write_some(buffers,
        std::forward<WriteHandler>(handler));
}

template<class Stream, class DynamicBuffer>
template<class MutableBufferSequence>
std::size_t
buffered_read_stream<Stream, DynamicBuffer>::
read_some(
    MutableBufferSequence const& buffers)
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

template<class Stream, class DynamicBuffer>
template<class MutableBufferSequence>
std::size_t
buffered_read_stream<Stream, DynamicBuffer>::
read_some(MutableBufferSequence const& buffers,
    error_code& ec)
{
    static_assert(is_sync_read_stream<next_layer_type>::value,
        "SyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    if(buffer_.size() == 0)
    {
        if(capacity_ == 0)
            return next_layer_.read_some(buffers, ec);
        buffer_.commit(next_layer_.read_some(
            buffer_.prepare(read_size(buffer_,
                capacity_)), ec));
        if(ec)
            return 0;
    }
    else
    {
        ec = {};
    }
    auto bytes_transferred =
        net::buffer_copy(buffers, buffer_.data());
    buffer_.consume(bytes_transferred);
    return bytes_transferred;
}

template<class Stream, class DynamicBuffer>
template<class MutableBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
buffered_read_stream<Stream, DynamicBuffer>::
async_read_some(
    MutableBufferSequence const& buffers,
    ReadHandler&& handler)
{
    static_assert(is_async_read_stream<next_layer_type>::value,
        "AsyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
            "MutableBufferSequence type requirements not met");
    if(buffer_.size() == 0 && capacity_ == 0)
        return next_layer_.async_read_some(buffers,
            std::forward<ReadHandler>(handler));
    return net::async_initiate<
        ReadHandler,
        void(error_code, std::size_t)>(
            typename ops::run_read_op{},
            handler,
            this,
            &buffers);
}

} // beast
} // boost

#endif
