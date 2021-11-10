//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_DETAIL_IMPL_READ_HPP
#define BOOST_BEAST_DETAIL_IMPL_READ_HPP

#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/read_size.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/throw_exception.hpp>

namespace boost {
namespace beast {
namespace detail {

// The number of bytes in the stack buffer when using non-blocking.
static std::size_t constexpr default_max_stack_buffer = 16384;

//------------------------------------------------------------------------------

struct dynamic_read_ops
{

// read into a dynamic buffer until the
// condition is met or an error occurs
template<
    class Stream,
    class DynamicBuffer,
    class Condition,
    class Handler>
class read_op
    : public asio::coroutine
    , public async_base<
        Handler, beast::executor_type<Stream>>
{
    Stream& s_;
    DynamicBuffer& b_;
    Condition cond_;
    error_code ec_;
    std::size_t total_ = 0;

public:
    read_op(read_op&&) = default;

    template<class Handler_, class Condition_>
    read_op(
        Handler_&& h,
        Stream& s,
        DynamicBuffer& b,
        Condition_&& cond)
        : async_base<Handler,
            beast::executor_type<Stream>>(
                std::forward<Handler_>(h),
                    s.get_executor())
        , s_(s)
        , b_(b)
        , cond_(std::forward<Condition_>(cond))
    {
        (*this)({}, 0, false);
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred,
        bool cont = true)
    {
        std::size_t max_prepare;
        BOOST_ASIO_CORO_REENTER(*this)
        {
            for(;;)
            {
                max_prepare = beast::read_size(b_, cond_(ec, total_, b_));
                if(max_prepare == 0)
                    break;
                BOOST_ASIO_CORO_YIELD
                s_.async_read_some(
                    b_.prepare(max_prepare), std::move(*this));
                b_.commit(bytes_transferred);
                total_ += bytes_transferred;
            }
            if(! cont)
            {
                // run this handler "as-if" using net::post
                // to reduce template instantiations
                ec_ = ec;
                BOOST_ASIO_CORO_YIELD
                s_.async_read_some(
                    b_.prepare(0), std::move(*this));
                ec = ec_;
            }
            this->complete_now(ec, total_);
        }
    }
};

//------------------------------------------------------------------------------

struct run_read_op
{
    template<
        class AsyncReadStream,
        class DynamicBuffer,
        class Condition,
        class ReadHandler>
    void
    operator()(
        ReadHandler&& h,
        AsyncReadStream* s,
        DynamicBuffer* b,
        Condition&& c)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<ReadHandler,
            void(error_code, std::size_t)>::value,
            "ReadHandler type requirements not met");

        read_op<
            AsyncReadStream,
            DynamicBuffer,
            typename std::decay<Condition>::type,
            typename std::decay<ReadHandler>::type>(
                std::forward<ReadHandler>(h),
                *s,
                *b,
                std::forward<Condition>(c));
    }

};

};

//------------------------------------------------------------------------------

template<
    class SyncReadStream,
    class DynamicBuffer,
    class CompletionCondition,
    class>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition cond)
{
    static_assert(is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value,
        "CompletionCondition type requirements not met");
    error_code ec;
    auto const bytes_transferred = detail::read(
        stream, buffer, std::move(cond), ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<
    class SyncReadStream,
    class DynamicBuffer,
    class CompletionCondition,
    class>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition cond,
    error_code& ec)
{
    static_assert(is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value,
        "CompletionCondition type requirements not met");
    ec = {};
    std::size_t total = 0;
    std::size_t max_prepare;
    for(;;)
    {
        max_prepare =  beast::read_size(buffer, cond(ec, total, buffer));
        if(max_prepare == 0)
            break;
        std::size_t const bytes_transferred =
            stream.read_some(buffer.prepare(max_prepare), ec);
        buffer.commit(bytes_transferred);
        total += bytes_transferred;
    }
    return total;
}

template<
    class AsyncReadStream,
    class DynamicBuffer,
    class CompletionCondition,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler,
    class>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    CompletionCondition&& cond,
    ReadHandler&& handler)
{
    static_assert(is_async_read_stream<AsyncReadStream>::value,
        "AsyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(
        detail::is_invocable<CompletionCondition,
            void(error_code&, std::size_t, DynamicBuffer&)>::value,
        "CompletionCondition type requirements not met");
    return net::async_initiate<
        ReadHandler,
        void(error_code, std::size_t)>(
            typename dynamic_read_ops::run_read_op{},
            handler,
            &stream,
            &buffer,
            std::forward<CompletionCondition>(cond));
}

} // detail
} // beast
} // boost

#endif
