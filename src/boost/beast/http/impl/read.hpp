//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
// Copyright (c) 2020 Richard Hodges (hodges.r@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_READ_HPP
#define BOOST_BEAST_HTTP_IMPL_READ_HPP

#include <boost/beast/http/type_traits.hpp>
#include <boost/beast/http/error.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/buffer.hpp>
#include <boost/beast/core/detail/read.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/compose.hpp>
#include <boost/asio/coroutine.hpp>

namespace boost {
namespace beast {
namespace http {

namespace detail {

struct parser_is_done
{
    template<bool isRequest>
    bool
    operator()(basic_parser<isRequest> const& p) const
    {
        return p.is_done();
    }
};

struct parser_is_header_done
{
    template<bool isRequest>
    bool
    operator()(basic_parser<isRequest> const& p) const
    {
        return p.is_header_done();
    }
};

//------------------------------------------------------------------------------

template<
    class Stream, class DynamicBuffer,
    bool isRequest, class Body, class Allocator,
    class Handler>
class read_msg_op
    : public beast::stable_async_base<
        Handler, beast::executor_type<Stream>>
    , public asio::coroutine
{
    using parser_type =
        parser<isRequest, Body, Allocator>;

    using message_type =
        typename parser_type::value_type;

    struct data
    {
        Stream& s;
        message_type& m;
        parser_type p;

        data(
            Stream& s_,
            message_type& m_)
            : s(s_)
            , m(m_)
            , p(std::move(m))
        {
        }
    };

    data& d_;

public:
    template<class Handler_>
    read_msg_op(
        Handler_&& h,
        Stream& s,
        DynamicBuffer& b,
        message_type& m)
        : stable_async_base<
            Handler, beast::executor_type<Stream>>(
                std::forward<Handler_>(h), s.get_executor())
        , d_(beast::allocate_stable<data>(
            *this, s, m))
    {
        BOOST_ASIO_HANDLER_LOCATION((
            __FILE__, __LINE__,
            "http::async_read(msg)"));

        http::async_read(d_.s, b, d_.p, std::move(*this));
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred)
    {
        if(! ec)
            d_.m = d_.p.release();
        this->complete_now(ec, bytes_transferred);
    }
};

struct run_read_msg_op
{
    template<
        class ReadHandler,
        class AsyncReadStream,
        class DynamicBuffer,
        bool isRequest, class Body, class Allocator>
    void
    operator()(
        ReadHandler&& h,
        AsyncReadStream* s,
        DynamicBuffer* b,
        message<isRequest, Body,
            basic_fields<Allocator>>* m)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<ReadHandler,
            void(error_code, std::size_t)>::value,
            "ReadHandler type requirements not met");

        read_msg_op<
            AsyncReadStream,
            DynamicBuffer,
            isRequest, Body, Allocator,
            typename std::decay<ReadHandler>::type>(
                std::forward<ReadHandler>(h), *s, *b, *m);
    }
};

template<class AsyncReadStream, class DynamicBuffer, bool isRequest>
class read_some_op : asio::coroutine
{
    AsyncReadStream& s_;
    DynamicBuffer& b_;
    basic_parser<isRequest>& p_;
    std::size_t bytes_transferred_;
    bool cont_;

public:
    read_some_op(
        AsyncReadStream& s,
        DynamicBuffer& b,
        basic_parser<isRequest>& p)
        : s_(s)
        , b_(b)
        , p_(p)
        , bytes_transferred_(0)
        , cont_(false)
    {
    }

    template<class Self>
    void operator()(
        Self& self,
        error_code ec = {},
        std::size_t bytes_transferred = 0)
    {
        BOOST_ASIO_CORO_REENTER(*this)
        {
            if(b_.size() == 0)
                goto do_read;
            for(;;)
            {
                // parse
                {
                    auto const used = p_.put(b_.data(), ec);
                    bytes_transferred_ += used;
                    b_.consume(used);
                }
                if(ec != http::error::need_more)
                    break;

            do_read:
                BOOST_ASIO_CORO_YIELD
                {
                    cont_ = true;
                    // VFALCO This was read_size_or_throw
                    auto const size = read_size(b_, 65536);
                    if(size == 0)
                    {
                        ec = error::buffer_overflow;
                        goto upcall;
                    }
                    auto const mb =
                        beast::detail::dynamic_buffer_prepare(
                            b_, size, ec, error::buffer_overflow);
                    if(ec)
                        goto upcall;

                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "http::async_read_some"));

                    s_.async_read_some(*mb, std::move(self));
                }
                b_.commit(bytes_transferred);
                if(ec == net::error::eof)
                {
                    BOOST_ASSERT(bytes_transferred == 0);
                    if(p_.got_some())
                    {
                        // caller sees EOF on next read
                        ec.assign(0, ec.category());
                        p_.put_eof(ec);
                        if(ec)
                            goto upcall;
                        BOOST_ASSERT(p_.is_done());
                        goto upcall;
                    }
                    ec = error::end_of_stream;
                    break;
                }
                if(ec)
                    break;
            }

        upcall:
            if(! cont_)
            {
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "http::async_read_some"));

                    net::post(
                        beast::bind_front_handler(std::move(self), ec));
                }
            }
            self.complete(ec, bytes_transferred_);
        }
    }
};

template<class Stream, class DynamicBuffer, bool isRequest, class Condition>
class read_op
    : asio::coroutine
{
    Stream& s_;
    DynamicBuffer& b_;
    basic_parser<isRequest>& p_;
    std::size_t bytes_transferred_;

public:
    read_op(Stream& s, DynamicBuffer& b, basic_parser<isRequest>& p)
    : s_(s)
    , b_(b)
    , p_(p)
    , bytes_transferred_(0)
    {
    }

    template<class Self>
    void operator()(Self& self, error_code ec = {}, std::size_t bytes_transferred = 0)
    {
        BOOST_ASIO_CORO_REENTER(*this)
        {
            if (Condition{}(p_))
            {
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "http::async_read"));

                    net::post(std::move(self));
                }
            }
            else
            {
                do
                {
                    BOOST_ASIO_CORO_YIELD
                    {
                        BOOST_ASIO_HANDLER_LOCATION((
                            __FILE__, __LINE__,
                            "http::async_read"));

                        async_read_some(
                            s_, b_, p_, std::move(self));
                    }
                    bytes_transferred_ += bytes_transferred;
                } while (!ec &&
                         !Condition{}(p_));
            }
            self.complete(ec, bytes_transferred_);
        }
    }
};


template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_some(SyncReadStream& s, DynamicBuffer& b, basic_parser<isRequest>& p, error_code& ec)
{
    std::size_t total = 0;
    ec.clear();
    if(b.size() == 0)
        goto do_read;
    for(;;)
    {
        // parse
        {
            auto const used = p.put(b.data(), ec);
            total += used;
            b.consume(used);
        }
        if(ec != http::error::need_more)
            break;

    do_read:
        // VFALCO This was read_size_or_throw
        auto const size = read_size(b, 65536);
        if(size == 0)
        {
            ec = error::buffer_overflow;
            return total;
        }
        auto const mb =
            beast::detail::dynamic_buffer_prepare(
                b, size, ec, error::buffer_overflow);
        if(ec)
            return total;
        std::size_t
            bytes_transferred =
                s.read_some(*mb, ec);
        b.commit(bytes_transferred);
        if(ec == net::error::eof)
        {
            BOOST_ASSERT(bytes_transferred == 0);
            if(p.got_some())
            {
                // caller sees EOF on next read
                ec.assign(0, ec.category());
                p.put_eof(ec);
                if(ec)
                    return total;
                BOOST_ASSERT(p.is_done());
                return total;
            }
            ec = error::end_of_stream;
            break;
        }
        if(ec)
            break;
    }

    return total;
}

template<class Condition, class Stream, class DynamicBuffer, bool isRequest>
std::size_t sync_read_op(Stream& s, DynamicBuffer& b, basic_parser<isRequest>& p, error_code& ec)
{
    std::size_t total = 0;
    ec.clear();

    if (!Condition{}(p))
    {
        do
        {
            total +=
                detail::read_some(s, b, p, ec);
        } while (!ec &&
                 !Condition{}(p));
    }
    return total;
}

} // detail

//------------------------------------------------------------------------------

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_some(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        http::read_some(stream, buffer, parser, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_some(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    return detail::read_some(stream, buffer, parser, ec);
}

template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read_some(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler)
{
    return net::async_compose<ReadHandler,
        void(beast::error_code, std::size_t)>(
            detail::read_some_op<AsyncReadStream, DynamicBuffer, isRequest> {
                stream,
                buffer,
                parser
            },
            handler,
            stream);
}

//------------------------------------------------------------------------------

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_header(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        http::read_header(stream, buffer, parser, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read_header(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    parser.eager(false);
    return detail::sync_read_op<
        detail::parser_is_header_done>(
            stream, buffer, parser, ec);
}

template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read_header(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler)
{
    parser.eager(false);
    return net::async_compose<
        ReadHandler,
        void(error_code, std::size_t)>(
        detail::read_op<
            AsyncReadStream,
            DynamicBuffer,
            isRequest,
            detail::parser_is_header_done>(
                stream, buffer, parser),
            handler, stream);
}

//------------------------------------------------------------------------------

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        http::read(stream, buffer, parser, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    error_code& ec)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    parser.eager(true);
    return detail::sync_read_op<
        detail::parser_is_done>(
            stream, buffer, parser, ec);
}

template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler)
{
    static_assert(
        is_async_read_stream<AsyncReadStream>::value,
        "AsyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    parser.eager(true);
    return net::async_compose<
        ReadHandler,
        void(error_code, std::size_t)>(
            detail::read_op<
                AsyncReadStream,
                DynamicBuffer,
                isRequest,
                detail::parser_is_done>(
                    stream, buffer, parser),
            handler, stream);
}

//------------------------------------------------------------------------------

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(is_body<Body>::value,
        "Body type requirements not met");
    static_assert(is_body_reader<Body>::value,
        "BodyReader type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        http::read(stream, buffer, msg, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<
    class SyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator>
std::size_t
read(
    SyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg,
    error_code& ec)
{
    static_assert(
        is_sync_read_stream<SyncReadStream>::value,
        "SyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(is_body<Body>::value,
        "Body type requirements not met");
    static_assert(is_body_reader<Body>::value,
        "BodyReader type requirements not met");
    parser<isRequest, Body, Allocator> p(std::move(msg));
    p.eager(true);
    auto const bytes_transferred =
        http::read(stream, buffer, p, ec);
    if(ec)
        return bytes_transferred;
    msg = p.release();
    return bytes_transferred;
}

template<
    class AsyncReadStream,
    class DynamicBuffer,
    bool isRequest, class Body, class Allocator,
    BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    message<isRequest, Body, basic_fields<Allocator>>& msg,
    ReadHandler&& handler)
{
    static_assert(
        is_async_read_stream<AsyncReadStream>::value,
        "AsyncReadStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    static_assert(is_body<Body>::value,
        "Body type requirements not met");
    static_assert(is_body_reader<Body>::value,
        "BodyReader type requirements not met");
    return net::async_initiate<
        ReadHandler,
        void(error_code, std::size_t)>(
            detail::run_read_msg_op{},
                handler, &stream, &buffer, &msg);
}

} // http
} // beast
} // boost

#endif
