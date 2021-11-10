//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_IMPL_STREAM_HPP
#define BOOST_BEAST_TEST_IMPL_STREAM_HPP

#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/detail/service_base.hpp>
#include <boost/beast/core/detail/is_invocable.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/post.hpp>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace boost {
namespace beast {
namespace test {

//------------------------------------------------------------------------------

template<class Executor>
template<class Handler, class Buffers>
class basic_stream<Executor>::read_op : public detail::stream_read_op_base
{
    using ex1_type =
        executor_type;
    using ex2_type
        = net::associated_executor_t<Handler, ex1_type>;

    struct lambda
    {
        Handler h_;
        boost::weak_ptr<detail::stream_state> wp_;
        Buffers b_;
#if defined(BOOST_ASIO_NO_TS_EXECUTORS)
        net::any_io_executor wg2_;
#else // defined(BOOST_ASIO_NO_TS_EXECUTORS)
        net::executor_work_guard<ex2_type> wg2_;
#endif // defined(BOOST_ASIO_NO_TS_EXECUTORS)

        lambda(lambda&&) = default;
        lambda(lambda const&) = default;

        template<class Handler_>
        lambda(
            Handler_&& h,
            boost::shared_ptr<detail::stream_state> const& s,
            Buffers const& b)
            : h_(std::forward<Handler_>(h))
            , wp_(s)
            , b_(b)
#if defined(BOOST_ASIO_NO_TS_EXECUTORS)
            , wg2_(net::prefer(
                net::get_associated_executor(
                  h_, s->exec),
                net::execution::outstanding_work.tracked))
#else // defined(BOOST_ASIO_NO_TS_EXECUTORS)
            , wg2_(net::get_associated_executor(
                h_, s->exec))
#endif // defined(BOOST_ASIO_NO_TS_EXECUTORS)
        {
        }

        using allocator_type = net::associated_allocator_t<Handler>;

        allocator_type get_allocator() const noexcept
        {
          return net::get_associated_allocator(h_);
        }

        void
        operator()(error_code ec)
        {
            std::size_t bytes_transferred = 0;
            auto sp = wp_.lock();
            if(! sp)
                ec = net::error::operation_aborted;
            if(! ec)
            {
                std::lock_guard<std::mutex> lock(sp->m);
                BOOST_ASSERT(! sp->op);
                if(sp->b.size() > 0)
                {
                    bytes_transferred =
                        net::buffer_copy(
                            b_, sp->b.data(), sp->read_max);
                    sp->b.consume(bytes_transferred);
                    sp->nread_bytes += bytes_transferred;
                }
                else if (buffer_bytes(b_) > 0)
                {
                    ec = net::error::eof;
                }
            }

#if defined(BOOST_ASIO_NO_TS_EXECUTORS)
            net::dispatch(wg2_,
                beast::bind_front_handler(std::move(h_),
                    ec, bytes_transferred));
            wg2_ = net::any_io_executor(); // probably unnecessary
#else // defined(BOOST_ASIO_NO_TS_EXECUTORS)
            net::dispatch(wg2_.get_executor(),
                beast::bind_front_handler(std::move(h_),
                    ec, bytes_transferred));
            wg2_.reset();
#endif // defined(BOOST_ASIO_NO_TS_EXECUTORS)
        }
    };

    lambda fn_;
#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
    net::executor_work_guard<net::any_io_executor> wg1_;
#else
    net::any_io_executor wg1_;
#endif

public:
    template<class Handler_>
    read_op(
        Handler_&& h,
        boost::shared_ptr<detail::stream_state> const& s,
        Buffers const& b)
        : fn_(std::forward<Handler_>(h), s, b)
#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
        , wg1_(s->exec)
#else
        , wg1_(net::prefer(s->exec,
            net::execution::outstanding_work.tracked))
#endif
    {
    }

    void
    operator()(error_code ec) override
    {
#if defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)
        net::post(wg1_.get_executor(),
            beast::bind_front_handler(std::move(fn_), ec));
        wg1_.reset();
#else
        net::post(wg1_, beast::bind_front_handler(std::move(fn_), ec));
        wg1_ = net::any_io_executor(); // probably unnecessary
#endif
    }
};

template<class Executor>
struct basic_stream<Executor>::run_read_op
{
    template<
        class ReadHandler,
        class MutableBufferSequence>
    void
    operator()(
        ReadHandler&& h,
        boost::shared_ptr<detail::stream_state> const& in,
        MutableBufferSequence const& buffers)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<ReadHandler,
                void(error_code, std::size_t)>::value,
            "ReadHandler type requirements not met");

        initiate_read(
            in,
            std::unique_ptr<detail::stream_read_op_base>{
            new read_op<
                typename std::decay<ReadHandler>::type,
                MutableBufferSequence>(
                    std::move(h),
                    in,
                    buffers)},
            buffer_bytes(buffers));
    }
};

template<class Executor>
struct basic_stream<Executor>::run_write_op
{
    template<
        class WriteHandler,
        class ConstBufferSequence>
    void
    operator()(
        WriteHandler&& h,
        boost::shared_ptr<detail::stream_state> in_,
        boost::weak_ptr<detail::stream_state> out_,
        ConstBufferSequence const& buffers)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<WriteHandler,
                void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");

        ++in_->nwrite;
        auto const upcall = [&](error_code ec, std::size_t n)
        {
            net::post(
                in_->exec,
                beast::bind_front_handler(std::move(h), ec, n));
        };

        // test failure
        error_code ec;
        std::size_t n = 0;
        if(in_->fc && in_->fc->fail(ec))
            return upcall(ec, n);

        // A request to write 0 bytes to a stream is a no-op.
        if(buffer_bytes(buffers) == 0)
            return upcall(ec, n);

        // connection closed
        auto out = out_.lock();
        if(! out)
            return upcall(net::error::connection_reset, n);

        // copy buffers
        n = std::min<std::size_t>(
            buffer_bytes(buffers), in_->write_max);
        {
            std::lock_guard<std::mutex> lock(out->m);
            n = net::buffer_copy(out->b.prepare(n), buffers);
            out->b.commit(n);
            out->nwrite_bytes += n;
            out->notify_read();
        }
        BOOST_ASSERT(! ec);
        upcall(ec, n);
    }
};

//------------------------------------------------------------------------------

template<class Executor>
template<class MutableBufferSequence>
std::size_t
basic_stream<Executor>::
read_some(MutableBufferSequence const& buffers)
{
    static_assert(net::is_mutable_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");
    error_code ec;
    auto const n = read_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return n;
}

template<class Executor>
template<class MutableBufferSequence>
std::size_t
basic_stream<Executor>::
read_some(MutableBufferSequence const& buffers,
    error_code& ec)
{
    static_assert(net::is_mutable_buffer_sequence<
            MutableBufferSequence>::value,
        "MutableBufferSequence type requirements not met");

    ++in_->nread;

    // test failure
    if(in_->fc && in_->fc->fail(ec))
        return 0;

    // A request to read 0 bytes from a stream is a no-op.
    if(buffer_bytes(buffers) == 0)
    {
        ec = {};
        return 0;
    }

    std::unique_lock<std::mutex> lock{in_->m};
    BOOST_ASSERT(! in_->op);
    in_->cv.wait(lock,
        [&]()
        {
            return
                in_->b.size() > 0 ||
                in_->code != detail::stream_status::ok;
        });

    // deliver bytes before eof
    if(in_->b.size() > 0)
    {
        auto const n = net::buffer_copy(
            buffers, in_->b.data(), in_->read_max);
        in_->b.consume(n);
        in_->nread_bytes += n;
        return n;
    }

    // deliver error
    BOOST_ASSERT(in_->code != detail::stream_status::ok);
    ec = net::error::eof;
    return 0;
}

template<class Executor>
template<class MutableBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, std::size_t)) ReadHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(error_code, std::size_t))
basic_stream<Executor>::
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
            run_read_op{},
            handler,
            in_,
            buffers);
}

template<class Executor>
template<class ConstBufferSequence>
std::size_t
basic_stream<Executor>::
write_some(ConstBufferSequence const& buffers)
{
    static_assert(net::is_const_buffer_sequence<
            ConstBufferSequence>::value,
        "ConstBufferSequence type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        write_some(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<class Executor>
template<class ConstBufferSequence>
std::size_t
basic_stream<Executor>::
write_some(
    ConstBufferSequence const& buffers, error_code& ec)
{
    static_assert(net::is_const_buffer_sequence<
            ConstBufferSequence>::value,
        "ConstBufferSequence type requirements not met");

    ++in_->nwrite;

    // test failure
    if(in_->fc && in_->fc->fail(ec))
        return 0;

    // A request to write 0 bytes to a stream is a no-op.
    if(buffer_bytes(buffers) == 0)
    {
        ec = {};
        return 0;
    }

    // connection closed
    auto out = out_.lock();
    if(! out)
    {
        ec = net::error::connection_reset;
        return 0;
    }

    // copy buffers
    auto n = std::min<std::size_t>(
        buffer_bytes(buffers), in_->write_max);
    {
        std::lock_guard<std::mutex> lock(out->m);
        n = net::buffer_copy(out->b.prepare(n), buffers);
        out->b.commit(n);
        out->nwrite_bytes += n;
        out->notify_read();
    }
    return n;
}

template<class Executor>
template<class ConstBufferSequence,
    BOOST_ASIO_COMPLETION_TOKEN_FOR(void(error_code, std::size_t)) WriteHandler>
BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(error_code, std::size_t))
basic_stream<Executor>::
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
            run_write_op{},
            handler,
            in_,
            out_,
            buffers);
}

//------------------------------------------------------------------------------

template<class Executor, class TeardownHandler>
void
async_teardown(
    role_type,
    basic_stream<Executor>& s,
    TeardownHandler&& handler)
{
    error_code ec;
    if( s.in_->fc &&
        s.in_->fc->fail(ec))
        return net::post(
            s.get_executor(),
            beast::bind_front_handler(
                std::move(handler), ec));
    s.close();
    if( s.in_->fc &&
        s.in_->fc->fail(ec))
        ec = net::error::eof;
    else
        ec = {};

    net::post(
        s.get_executor(),
        beast::bind_front_handler(
            std::move(handler), ec));
}

//------------------------------------------------------------------------------

template<class Executor, class Arg1, class... ArgN>
basic_stream<Executor>
connect(stream& to, Arg1&& arg1, ArgN&&... argn)
{
    stream from{
        std::forward<Arg1>(arg1),
        std::forward<ArgN>(argn)...};
    from.connect(to);
    return from;
}

namespace detail
{
template<class To>
struct extract_executor_op
{
    To operator()(net::any_io_executor& ex) const
    {
        assert(ex.template target<To>());
        return *ex.template target<To>();
    }
};

template<>
struct extract_executor_op<net::any_io_executor>
{
    net::any_io_executor operator()(net::any_io_executor& ex) const
    {
        return ex;
    }
};
}

template<class Executor>
auto basic_stream<Executor>::get_executor() noexcept -> executor_type
{
    return detail::extract_executor_op<Executor>()(in_->exec);
}

} // test
} // beast
} // boost

#endif
