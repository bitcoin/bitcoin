//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_FLAT_STREAM_HPP
#define BOOST_BEAST_CORE_IMPL_FLAT_STREAM_HPP

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/core/static_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/websocket/teardown.hpp>
#include <boost/asio/buffer.hpp>
#include <memory>

namespace boost {
namespace beast {

template<class NextLayer>
struct flat_stream<NextLayer>::ops
{

template<class Handler>
class write_op
    : public async_base<Handler,
        beast::executor_type<flat_stream>>
{
public:
    template<
        class ConstBufferSequence,
        class Handler_>
    write_op(
        Handler_&& h,
        flat_stream<NextLayer>& s,
        ConstBufferSequence const& b)
        : async_base<Handler,
            beast::executor_type<flat_stream>>(
                std::forward<Handler_>(h),
                s.get_executor())
    {
        auto const result =
            flatten(b, max_size);
        if(result.flatten)
        {
            s.buffer_.clear();
            s.buffer_.commit(net::buffer_copy(
                s.buffer_.prepare(result.size),
                b, result.size));

            BOOST_ASIO_HANDLER_LOCATION((
                __FILE__, __LINE__,
                "flat_stream::async_write_some"));

            s.stream_.async_write_some(
                s.buffer_.data(), std::move(*this));
        }
        else
        {
            s.buffer_.clear();
            s.buffer_.shrink_to_fit();

            BOOST_ASIO_HANDLER_LOCATION((
                __FILE__, __LINE__,
                "flat_stream::async_write_some"));

            s.stream_.async_write_some(
                beast::buffers_prefix(
                    result.size, b), std::move(*this));
        }
    }

    void
    operator()(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        this->complete_now(ec, bytes_transferred);
    }
};

struct run_write_op
{
    template<class WriteHandler, class Buffers>
    void
    operator()(
        WriteHandler&& h,
        flat_stream* s,
        Buffers const& b)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<WriteHandler,
            void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");

        write_op<
            typename std::decay<WriteHandler>::type>(
                std::forward<WriteHandler>(h), *s, b);
    }
};

};

//------------------------------------------------------------------------------

template<class NextLayer>
template<class... Args>
flat_stream<NextLayer>::
flat_stream(Args&&... args)
    : stream_(std::forward<Args>(args)...)
{
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
flat_stream<NextLayer>::
read_some(MutableBufferSequence const& buffers)
{
    static_assert(boost::beast::is_sync_read_stream<next_layer_type>::value,
        "SyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");
    error_code ec;
    auto n = read_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
    return n;
}

template<class NextLayer>
template<class MutableBufferSequence>
std::size_t
flat_stream<NextLayer>::
read_some(MutableBufferSequence const& buffers, error_code& ec)
{
    static_assert(boost::beast::is_sync_read_stream<next_layer_type>::value,
        "SyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");
    return stream_.read_some(buffers, ec);
}

template<class NextLayer>
template<
    class MutableBufferSequence,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
flat_stream<NextLayer>::
async_read_some(
    MutableBufferSequence const& buffers,
    ReadHandler&& handler)
{
    static_assert(boost::beast::is_async_read_stream<next_layer_type>::value,
        "AsyncReadStream type requirements not met");
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence >::value,
        "MutableBufferSequence type requirements not met");
    return stream_.async_read_some(
        buffers, std::forward<ReadHandler>(handler));
}

template<class NextLayer>
template<class ConstBufferSequence>
std::size_t
flat_stream<NextLayer>::
write_some(ConstBufferSequence const& buffers)
{
    static_assert(boost::beast::is_sync_write_stream<next_layer_type>::value,
        "SyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    error_code ec;
    auto n = write_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
    return n;
}

template<class NextLayer>
template<class ConstBufferSequence>
std::size_t
flat_stream<NextLayer>::
stack_write_some(
    std::size_t size,
    ConstBufferSequence const& buffers,
    error_code& ec)
{
    static_buffer<max_stack> b;
    b.commit(net::buffer_copy(
        b.prepare(size), buffers));
    return stream_.write_some(b.data(), ec);
}

template<class NextLayer>
template<class ConstBufferSequence>
std::size_t
flat_stream<NextLayer>::
write_some(ConstBufferSequence const& buffers, error_code& ec)
{
    static_assert(boost::beast::is_sync_write_stream<next_layer_type>::value,
        "SyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
        "ConstBufferSequence type requirements not met");
    auto const result = flatten(buffers, max_size);
    if(result.flatten)
    {
        if(result.size <= max_stack)
            return stack_write_some(result.size, buffers, ec);

        buffer_.clear();
        buffer_.commit(net::buffer_copy(
            buffer_.prepare(result.size),
            buffers));
        return stream_.write_some(buffer_.data(), ec);
    }
    buffer_.clear();
    buffer_.shrink_to_fit();
    return stream_.write_some(
        boost::beast::buffers_prefix(result.size, buffers), ec);
}

template<class NextLayer>
template<
    class ConstBufferSequence,
    BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
flat_stream<NextLayer>::
async_write_some(
    ConstBufferSequence const& buffers,
    WriteHandler&& handler)
{
    static_assert(boost::beast::is_async_write_stream<next_layer_type>::value,
        "AsyncWriteStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
        "ConstBufferSequence type requirements not met");
    return net::async_initiate<
        WriteHandler,
        void(error_code, std::size_t)>(
            typename ops::run_write_op{},
            handler,
            this,
            buffers);
}

template<class NextLayer>
void
teardown(
    boost::beast::role_type role,
    flat_stream<NextLayer>& s,
    error_code& ec)
{
    using boost::beast::websocket::teardown;
    teardown(role, s.next_layer(), ec);
}

template<class NextLayer, class TeardownHandler>
void
async_teardown(
    boost::beast::role_type role,
    flat_stream<NextLayer>& s,
    TeardownHandler&& handler)
{
    using boost::beast::websocket::async_teardown;
    async_teardown(role, s.next_layer(), std::move(handler));
}

} // beast
} // boost

#endif
