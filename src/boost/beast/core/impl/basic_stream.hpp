//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_IMPL_BASIC_STREAM_HPP
#define BOOST_BEAST_CORE_IMPL_BASIC_STREAM_HPP

#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/websocket/teardown.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/assert.hpp>
#include <boost/make_shared.hpp>
#include <boost/core/exchange.hpp>
#include <cstdlib>
#include <type_traits>
#include <utility>

namespace boost {
namespace beast {

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
template<class... Args>
basic_stream<Protocol, Executor, RatePolicy>::
impl_type::
impl_type(std::false_type, Args&&... args)
    : socket(std::forward<Args>(args)...)
    , read(ex())
    , write(ex())
    , timer(ex())
{
    reset();
}

template<class Protocol, class Executor, class RatePolicy>
template<class RatePolicy_, class... Args>
basic_stream<Protocol, Executor, RatePolicy>::
impl_type::
impl_type(std::true_type,
    RatePolicy_&& policy, Args&&... args)
    : boost::empty_value<RatePolicy>(
        boost::empty_init_t{},
        std::forward<RatePolicy_>(policy))
    , socket(std::forward<Args>(args)...)
    , read(ex())
    , write(ex())
    , timer(ex())
{
    reset();
}

template<class Protocol, class Executor, class RatePolicy>
template<class Executor2>
void
basic_stream<Protocol, Executor, RatePolicy>::
impl_type::
on_timer(Executor2 const& ex2)
{
    BOOST_ASSERT(waiting > 0);

    // the last waiter starts the new slice
    if(--waiting > 0)
        return;

    // update the expiration time
    BOOST_VERIFY(timer.expires_after(
        std::chrono::seconds(1)) == 0);

    rate_policy_access::on_timer(policy());

    struct handler : boost::empty_value<Executor2>
    {
        boost::weak_ptr<impl_type> wp;

        using executor_type = Executor2;

        executor_type
        get_executor() const noexcept
        {
            return this->get();
        }

        handler(
            Executor2 const& ex2,
            boost::shared_ptr<impl_type> const& sp)
            : boost::empty_value<Executor2>(
                boost::empty_init_t{}, ex2)
            , wp(sp)
        {
        }

        void
        operator()(error_code ec)
        {
            auto sp = wp.lock();
            if(! sp)
                return;
            if(ec == net::error::operation_aborted)
                return;
            BOOST_ASSERT(! ec);
            if(ec)
                return;
            sp->on_timer(this->get());
        }
    };

    // wait on the timer again
    ++waiting;
    timer.async_wait(handler(ex2, this->shared_from_this()));
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
impl_type::
reset()
{
    // If assert goes off, it means that there are
    // already read or write (or connect) operations
    // outstanding, so there is nothing to apply
    // the expiration time to!
    //
    BOOST_ASSERT(! read.pending || ! write.pending);

    if(! read.pending)
        BOOST_VERIFY(
            read.timer.expires_at(never()) == 0);

    if(! write.pending)
        BOOST_VERIFY(
            write.timer.expires_at(never()) == 0);
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
impl_type::
close() noexcept
{
    {
        error_code ec;
        socket.close(ec);
    }
    try
    {
        timer.cancel();
    }
    catch(...)
    {
    }
}

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
template<class Executor2>
struct basic_stream<Protocol, Executor, RatePolicy>::
    timeout_handler
{
    using executor_type = Executor2;

    op_state& state;
    boost::weak_ptr<impl_type> wp;
    tick_type tick;
    executor_type ex;

    executor_type get_executor() const noexcept
    {
        return ex;
    }

    void
    operator()(error_code ec)
    {
        // timer canceled
        if(ec == net::error::operation_aborted)
            return;
        BOOST_ASSERT(! ec);

        auto sp = wp.lock();

        // stream destroyed
        if(! sp)
            return;

        // stale timer
        if(tick < state.tick)
            return;
        BOOST_ASSERT(tick == state.tick);

        // timeout
        BOOST_ASSERT(! state.timeout);
        sp->close();
        state.timeout = true;
    }
};

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
struct basic_stream<Protocol, Executor, RatePolicy>::ops
{

template<bool isRead, class Buffers, class Handler>
class transfer_op
    : public async_base<Handler, Executor>
    , public boost::asio::coroutine
{
    boost::shared_ptr<impl_type> impl_;
    pending_guard pg_;
    Buffers b_;

    using is_read = std::integral_constant<bool, isRead>;

    op_state&
    state()
    {
        if (isRead)
            return impl_->read;
        else
            return impl_->write;
    }

    std::size_t
    available_bytes()
    {
        if (isRead)
            return rate_policy_access::
                available_read_bytes(impl_->policy());
        else
            return rate_policy_access::
                available_write_bytes(impl_->policy());
    }

    void
    transfer_bytes(std::size_t n)
    {
        if (isRead)
            rate_policy_access::
                transfer_read_bytes(impl_->policy(), n);
        else
            rate_policy_access::
                transfer_write_bytes(impl_->policy(), n);
    }

    void
    async_perform(
        std::size_t amount, std::true_type)
    {
        impl_->socket.async_read_some(
            beast::buffers_prefix(amount, b_),
                std::move(*this));
    }

    void
    async_perform(
        std::size_t amount, std::false_type)
    {
        impl_->socket.async_write_some(
            beast::buffers_prefix(amount, b_),
                std::move(*this));
    }

    static bool never_pending_;

public:
    template<class Handler_>
    transfer_op(
        Handler_&& h,
        basic_stream& s,
        Buffers const& b)
        : async_base<Handler, Executor>(
            std::forward<Handler_>(h), s.get_executor())
        , impl_(s.impl_)
        , pg_()
        , b_(b)
    {
        if (buffer_bytes(b_) == 0 && state().pending)
        {
            // Workaround:
            // Corner case discovered in https://github.com/boostorg/beast/issues/2065
            // Enclosing SSL stream wishes to complete a 0-length write early by
            // executing a 0-length read against the underlying stream.
            // This can occur even if an existing async_read is in progress.
            // In this specific case, we will complete the async op with no error
            // in order to prevent assertions and/or internal corruption of the basic_stream
            this->complete(false, error_code(), 0);
        }
        else
        {
            pg_.assign(state().pending);
            (*this)({});
        }
    }

    void
    operator()(
        error_code ec,
        std::size_t bytes_transferred = 0)
    {
        BOOST_ASIO_CORO_REENTER(*this)
        {
            // handle empty buffers
            if(detail::buffers_empty(b_))
            {
                // make sure we perform the no-op
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        (isRead ? "basic_stream::async_read_some"
                            : "basic_stream::async_write_some")));

                    async_perform(0, is_read{});
                }
                // apply the timeout manually, otherwise
                // behavior varies across platforms.
                if(state().timer.expiry() <= clock_type::now())
                {
                    impl_->close();
                    ec = beast::error::timeout;
                }
                goto upcall;
            }

            // if a timeout is active, wait on the timer
            if(state().timer.expiry() != never())
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    (isRead ? "basic_stream::async_read_some"
                        : "basic_stream::async_write_some")));

                state().timer.async_wait(
                    timeout_handler<decltype(this->get_executor())>{
                        state(),
                        impl_,
                        state().tick,
                        this->get_executor()});
            }

            // check rate limit, maybe wait
            std::size_t amount;
            amount = available_bytes();
            if(amount == 0)
            {
                ++impl_->waiting;
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        (isRead ? "basic_stream::async_read_some"
                            : "basic_stream::async_write_some")));

                    impl_->timer.async_wait(std::move(*this));
                }
                if(ec)
                {
                    // socket was closed, or a timeout
                    BOOST_ASSERT(ec ==
                        net::error::operation_aborted);
                    // timeout handler invoked?
                    if(state().timeout)
                    {
                        // yes, socket already closed
                        ec = beast::error::timeout;
                        state().timeout = false;
                    }
                    goto upcall;
                }
                impl_->on_timer(this->get_executor());

                // Allow at least one byte, otherwise
                // bytes_transferred could be 0.
                amount = std::max<std::size_t>(
                    available_bytes(), 1);
            }

            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    (isRead ? "basic_stream::async_read_some"
                        : "basic_stream::async_write_some")));

                async_perform(amount, is_read{});
            }

            if(state().timer.expiry() != never())
            {
                ++state().tick;

                // try cancelling timer
                auto const n =
                    state().timer.cancel();
                if(n == 0)
                {
                    // timeout handler invoked?
                    if(state().timeout)
                    {
                        // yes, socket already closed
                        ec = beast::error::timeout;
                        state().timeout = false;
                    }
                }
                else
                {
                    BOOST_ASSERT(n == 1);
                    BOOST_ASSERT(! state().timeout);
                }
            }

        upcall:
            pg_.reset();
            transfer_bytes(bytes_transferred);
            this->complete_now(ec, bytes_transferred);
        }
    }
};

template<class Handler>
class connect_op
    : public async_base<Handler, Executor>
{
    boost::shared_ptr<impl_type> impl_;
    pending_guard pg0_;
    pending_guard pg1_;

    op_state&
    state() noexcept
    {
        return impl_->write;
    }

public:
    template<class Handler_>
    connect_op(
        Handler_&& h,
        basic_stream& s,
        endpoint_type ep)
        : async_base<Handler, Executor>(
            std::forward<Handler_>(h), s.get_executor())
        , impl_(s.impl_)
        , pg0_(impl_->read.pending)
        , pg1_(impl_->write.pending)
    {
        if(state().timer.expiry() != stream_base::never())
        {
            BOOST_ASIO_HANDLER_LOCATION((
                __FILE__, __LINE__,
                "basic_stream::async_connect"));

            impl_->write.timer.async_wait(
                timeout_handler<decltype(this->get_executor())>{
                    state(),
                    impl_,
                    state().tick,
                    this->get_executor()});
        }

        BOOST_ASIO_HANDLER_LOCATION((
            __FILE__, __LINE__,
            "basic_stream::async_connect"));

        impl_->socket.async_connect(
            ep, std::move(*this));
        // *this is now moved-from
    }

    template<
        class Endpoints, class Condition,
        class Handler_>
    connect_op(
        Handler_&& h,
        basic_stream& s,
        Endpoints const& eps,
        Condition const& cond)
        : async_base<Handler, Executor>(
            std::forward<Handler_>(h), s.get_executor())
        , impl_(s.impl_)
        , pg0_(impl_->read.pending)
        , pg1_(impl_->write.pending)
    {
        if(state().timer.expiry() != stream_base::never())
        {
            BOOST_ASIO_HANDLER_LOCATION((
                __FILE__, __LINE__,
                "basic_stream::async_connect"));

            impl_->write.timer.async_wait(
                timeout_handler<decltype(this->get_executor())>{
                    state(),
                    impl_,
                    state().tick,
                    this->get_executor()});
        }

        BOOST_ASIO_HANDLER_LOCATION((
            __FILE__, __LINE__,
            "basic_stream::async_connect"));

        net::async_connect(impl_->socket,
            eps, cond, std::move(*this));
        // *this is now moved-from
    }

    template<
        class Iterator, class Condition,
        class Handler_>
    connect_op(
        Handler_&& h,
        basic_stream& s,
        Iterator begin, Iterator end,
        Condition const& cond)
        : async_base<Handler, Executor>(
            std::forward<Handler_>(h), s.get_executor())
        , impl_(s.impl_)
        , pg0_(impl_->read.pending)
        , pg1_(impl_->write.pending)
    {
        if(state().timer.expiry() != stream_base::never())
        {
            BOOST_ASIO_HANDLER_LOCATION((
                __FILE__, __LINE__,
                "basic_stream::async_connect"));

            impl_->write.timer.async_wait(
                timeout_handler<decltype(this->get_executor())>{
                    state(),
                    impl_,
                    state().tick,
                    this->get_executor()});
        }

        BOOST_ASIO_HANDLER_LOCATION((
            __FILE__, __LINE__,
            "basic_stream::async_connect"));

        net::async_connect(impl_->socket,
            begin, end, cond, std::move(*this));
        // *this is now moved-from
    }

    template<class... Args>
    void
    operator()(error_code ec, Args&&... args)
    {
        if(state().timer.expiry() != stream_base::never())
        {
            ++state().tick;

            // try cancelling timer
            auto const n =
                impl_->write.timer.cancel();
            if(n == 0)
            {
                // timeout handler invoked?
                if(state().timeout)
                {
                    // yes, socket already closed
                    ec = beast::error::timeout;
                    state().timeout = false;
                }
            }
            else
            {
                BOOST_ASSERT(n == 1);
                BOOST_ASSERT(! state().timeout);
            }
        }

        pg0_.reset();
        pg1_.reset();
        this->complete_now(ec, std::forward<Args>(args)...);
    }
};

struct run_read_op
{
    template<class ReadHandler, class Buffers>
    void
    operator()(
        ReadHandler&& h,
        basic_stream* s,
        Buffers const& b)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            detail::is_invocable<ReadHandler,
                void(error_code, std::size_t)>::value,
            "ReadHandler type requirements not met");

        transfer_op<
            true,
            Buffers,
            typename std::decay<ReadHandler>::type>(
                std::forward<ReadHandler>(h), *s, b);
    }
};

struct run_write_op
{
    template<class WriteHandler, class Buffers>
    void
    operator()(
        WriteHandler&& h,
        basic_stream* s,
        Buffers const& b)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            detail::is_invocable<WriteHandler,
                void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");

        transfer_op<
            false,
            Buffers,
            typename std::decay<WriteHandler>::type>(
                std::forward<WriteHandler>(h), *s, b);
    }
};

struct run_connect_op
{
    template<class ConnectHandler>
    void
    operator()(
        ConnectHandler&& h,
        basic_stream* s,
        endpoint_type const& ep)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            detail::is_invocable<ConnectHandler,
                void(error_code)>::value,
            "ConnectHandler type requirements not met");

        connect_op<typename std::decay<ConnectHandler>::type>(
            std::forward<ConnectHandler>(h), *s, ep);
    }
};

struct run_connect_range_op
{
    template<
        class RangeConnectHandler,
        class EndpointSequence,
        class Condition>
    void
    operator()(
        RangeConnectHandler&& h,
        basic_stream* s,
        EndpointSequence const& eps,
        Condition const& cond)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            detail::is_invocable<RangeConnectHandler,
                void(error_code, typename Protocol::endpoint)>::value,
            "RangeConnectHandler type requirements not met");

        connect_op<typename std::decay<RangeConnectHandler>::type>(
            std::forward<RangeConnectHandler>(h), *s, eps, cond);
    }
};

struct run_connect_iter_op
{
    template<
        class IteratorConnectHandler,
        class Iterator,
        class Condition>
    void
    operator()(
        IteratorConnectHandler&& h,
        basic_stream* s,
        Iterator begin, Iterator end,
        Condition const& cond)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            detail::is_invocable<IteratorConnectHandler,
                void(error_code, Iterator)>::value,
            "IteratorConnectHandler type requirements not met");

        connect_op<typename std::decay<IteratorConnectHandler>::type>(
            std::forward<IteratorConnectHandler>(h), *s, begin, end, cond);
    }
};

};

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
basic_stream<Protocol, Executor, RatePolicy>::
~basic_stream()
{
    // the shared object can outlive *this,
    // cancel any operations so the shared
    // object is destroyed as soon as possible.
    impl_->close();
}

template<class Protocol, class Executor, class RatePolicy>
template<class Arg0, class... Args, class>
basic_stream<Protocol, Executor, RatePolicy>::
basic_stream(Arg0&& arg0, Args&&... args)
    : impl_(boost::make_shared<impl_type>(
        std::false_type{},
        std::forward<Arg0>(arg0),
        std::forward<Args>(args)...))
{
}

template<class Protocol, class Executor, class RatePolicy>
template<class RatePolicy_, class Arg0, class... Args, class>
basic_stream<Protocol, Executor, RatePolicy>::
basic_stream(
    RatePolicy_&& policy, Arg0&& arg0, Args&&... args)
    : impl_(boost::make_shared<impl_type>(
        std::true_type{},
        std::forward<RatePolicy_>(policy),
        std::forward<Arg0>(arg0),
        std::forward<Args>(args)...))
{
}

template<class Protocol, class Executor, class RatePolicy>
basic_stream<Protocol, Executor, RatePolicy>::
basic_stream(basic_stream&& other)
    : impl_(boost::make_shared<impl_type>(
        std::move(*other.impl_)))
{
    // Explainer: Asio's sockets provide the guarantee that a moved-from socket
    // will be in a state as-if newly created. i.e.:
    // * having the same (valid) executor
    // * the socket shall not be open
    // We provide the same guarantee by moving the impl rather than the pointer
    // controlling its lifetime.
}

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
auto
basic_stream<Protocol, Executor, RatePolicy>::
release_socket() ->
    socket_type
{
    this->cancel();
    return std::move(impl_->socket);
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
expires_after(net::steady_timer::duration expiry_time)
{
    // If assert goes off, it means that there are
    // already read or write (or connect) operations
    // outstanding, so there is nothing to apply
    // the expiration time to!
    //
    BOOST_ASSERT(
        ! impl_->read.pending ||
        ! impl_->write.pending);

    if(! impl_->read.pending)
        BOOST_VERIFY(
            impl_->read.timer.expires_after(
                expiry_time) == 0);

    if(! impl_->write.pending)
        BOOST_VERIFY(
            impl_->write.timer.expires_after(
                expiry_time) == 0);
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
expires_at(
    net::steady_timer::time_point expiry_time)
{
    // If assert goes off, it means that there are
    // already read or write (or connect) operations
    // outstanding, so there is nothing to apply
    // the expiration time to!
    //
    BOOST_ASSERT(
        ! impl_->read.pending ||
        ! impl_->write.pending);

    if(! impl_->read.pending)
        BOOST_VERIFY(
            impl_->read.timer.expires_at(
                expiry_time) == 0);

    if(! impl_->write.pending)
        BOOST_VERIFY(
            impl_->write.timer.expires_at(
                expiry_time) == 0);
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
expires_never()
{
    impl_->reset();
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
cancel()
{
    error_code ec;
    impl_->socket.cancel(ec);
    impl_->timer.cancel();
}

template<class Protocol, class Executor, class RatePolicy>
void
basic_stream<Protocol, Executor, RatePolicy>::
close()
{
    impl_->close();
}

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
template<BOOST_BEAST_ASYNC_TPARAM1 ConnectHandler>
BOOST_BEAST_ASYNC_RESULT1(ConnectHandler)
basic_stream<Protocol, Executor, RatePolicy>::
async_connect(
    endpoint_type const& ep,
    ConnectHandler&& handler)
{
    return net::async_initiate<
        ConnectHandler,
        void(error_code)>(
            typename ops::run_connect_op{},
            handler,
            this,
            ep);
}

template<class Protocol, class Executor, class RatePolicy>
template<
    class EndpointSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, typename Protocol::endpoint)) RangeConnectHandler,
    class>
BOOST_ASIO_INITFN_RESULT_TYPE(RangeConnectHandler,void(error_code, typename Protocol::endpoint))
basic_stream<Protocol, Executor, RatePolicy>::
async_connect(
    EndpointSequence const& endpoints,
    RangeConnectHandler&& handler)
{
    return net::async_initiate<
        RangeConnectHandler,
        void(error_code, typename Protocol::endpoint)>(
            typename ops::run_connect_range_op{},
            handler,
            this,
            endpoints,
            detail::any_endpoint{});
}

template<class Protocol, class Executor, class RatePolicy>
template<
    class EndpointSequence,
    class ConnectCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, typename Protocol::endpoint)) RangeConnectHandler,
    class>
BOOST_ASIO_INITFN_RESULT_TYPE(RangeConnectHandler,void (error_code, typename Protocol::endpoint))
basic_stream<Protocol, Executor, RatePolicy>::
async_connect(
    EndpointSequence const& endpoints,
    ConnectCondition connect_condition,
    RangeConnectHandler&& handler)
{
    return net::async_initiate<
        RangeConnectHandler,
        void(error_code, typename Protocol::endpoint)>(
            typename ops::run_connect_range_op{},
            handler,
            this,
            endpoints,
            connect_condition);
}

template<class Protocol, class Executor, class RatePolicy>
template<
    class Iterator,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, Iterator)) IteratorConnectHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(IteratorConnectHandler,void (error_code, Iterator))
basic_stream<Protocol, Executor, RatePolicy>::
async_connect(
    Iterator begin, Iterator end,
    IteratorConnectHandler&& handler)
{
    return net::async_initiate<
        IteratorConnectHandler,
        void(error_code, Iterator)>(
            typename ops::run_connect_iter_op{},
            handler,
            this,
            begin, end,
            detail::any_endpoint{});
}

template<class Protocol, class Executor, class RatePolicy>
template<
    class Iterator,
    class ConnectCondition,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, Iterator)) IteratorConnectHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(IteratorConnectHandler,void (error_code, Iterator))
basic_stream<Protocol, Executor, RatePolicy>::
async_connect(
    Iterator begin, Iterator end,
    ConnectCondition connect_condition,
    IteratorConnectHandler&& handler)
{
    return net::async_initiate<
        IteratorConnectHandler,
        void(error_code, Iterator)>(
            typename ops::run_connect_iter_op{},
            handler,
            this,
            begin, end,
            connect_condition);
}

//------------------------------------------------------------------------------

template<class Protocol, class Executor, class RatePolicy>
template<class MutableBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 ReadHandler>
BOOST_BEAST_ASYNC_RESULT2(ReadHandler)
basic_stream<Protocol, Executor, RatePolicy>::
async_read_some(
    MutableBufferSequence const& buffers,
    ReadHandler&& handler)
{
    static_assert(net::is_mutable_buffer_sequence<
        MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");
    return net::async_initiate<
        ReadHandler,
        void(error_code, std::size_t)>(
            typename ops::run_read_op{},
            handler,
            this,
            buffers);
}

template<class Protocol, class Executor, class RatePolicy>
template<class ConstBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
basic_stream<Protocol, Executor, RatePolicy>::
async_write_some(
    ConstBufferSequence const& buffers,
    WriteHandler&& handler)
{
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

//------------------------------------------------------------------------------
//
// Customization points
//

#if ! BOOST_BEAST_DOXYGEN

template<
    class Protocol, class Executor, class RatePolicy>
void
beast_close_socket(
    basic_stream<Protocol, Executor, RatePolicy>& stream)
{
    error_code ec;
    stream.socket().close(ec);
}

template<
    class Protocol, class Executor, class RatePolicy>
void
teardown(
    role_type role,
    basic_stream<Protocol, Executor, RatePolicy>& stream,
    error_code& ec)
{
    using beast::websocket::teardown;
    teardown(role, stream.socket(), ec);
}

template<
    class Protocol, class Executor, class RatePolicy,
    class TeardownHandler>
void
async_teardown(
    role_type role,
    basic_stream<Protocol, Executor, RatePolicy>& stream,
    TeardownHandler&& handler)
{
    using beast::websocket::async_teardown;
    async_teardown(role, stream.socket(),
        std::forward<TeardownHandler>(handler));
}

#endif

} // beast
} // boost

#endif
