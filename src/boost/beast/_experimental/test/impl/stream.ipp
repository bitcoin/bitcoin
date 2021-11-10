//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_TEST_IMPL_STREAM_IPP
#define BOOST_BEAST_TEST_IMPL_STREAM_IPP

#include <boost/beast/_experimental/test/stream.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/make_shared.hpp>
#include <stdexcept>
#include <vector>

namespace boost {
namespace beast {
namespace test {

//------------------------------------------------------------------------------

template<class Executor>
void basic_stream<Executor>::initiate_read(
    boost::shared_ptr<detail::stream_state> const& in_,
    std::unique_ptr<detail::stream_read_op_base>&& op,
    std::size_t buf_size)
{
    std::unique_lock<std::mutex> lock(in_->m);

    ++in_->nread;
    if(in_->op != nullptr)
        BOOST_THROW_EXCEPTION(
            std::logic_error{"in_->op != nullptr"});

    // test failure
    error_code ec;
    if(in_->fc && in_->fc->fail(ec))
    {
        lock.unlock();
        (*op)(ec);
        return;
    }

    // A request to read 0 bytes from a stream is a no-op.
    if(buf_size == 0 || buffer_bytes(in_->b.data()) > 0)
    {
        lock.unlock();
        (*op)(ec);
        return;
    }

    // deliver error
    if(in_->code != detail::stream_status::ok)
    {
        lock.unlock();
        (*op)(net::error::eof);
        return;
    }

    // complete when bytes available or closed
    in_->op = std::move(op);
}

//------------------------------------------------------------------------------

template<class Executor>
basic_stream<Executor>::
~basic_stream()
{
    close();
    in_->remove();
}

template<class Executor>
basic_stream<Executor>::
basic_stream(basic_stream&& other)
{
    auto in = detail::stream_service::make_impl(
        other.in_->exec, other.in_->fc);
    in_ = std::move(other.in_);
    out_ = std::move(other.out_);
    other.in_ = in;
}


template<class Executor>
basic_stream<Executor>&
basic_stream<Executor>::
operator=(basic_stream&& other)
{
    close();
    auto in = detail::stream_service::make_impl(
        other.in_->exec, other.in_->fc);
    in_->remove();
    in_ = std::move(other.in_);
    out_ = std::move(other.out_);
    other.in_ = in;
    return *this;
}

//------------------------------------------------------------------------------

template<class Executor>
basic_stream<Executor>::
basic_stream(executor_type exec)
    : in_(detail::stream_service::make_impl(std::move(exec), nullptr))
{
}

template<class Executor>
basic_stream<Executor>::
basic_stream(
    net::io_context& ioc,
    fail_count& fc)
    : in_(detail::stream_service::make_impl(ioc.get_executor(), &fc))
{
}

template<class Executor>
basic_stream<Executor>::
basic_stream(
    net::io_context& ioc,
    string_view s)
    : in_(detail::stream_service::make_impl(ioc.get_executor(), nullptr))
{
    in_->b.commit(net::buffer_copy(
        in_->b.prepare(s.size()),
        net::buffer(s.data(), s.size())));
}

template<class Executor>
basic_stream<Executor>::
basic_stream(
    net::io_context& ioc,
    fail_count& fc,
    string_view s)
    : in_(detail::stream_service::make_impl(ioc.get_executor(), &fc))
{
    in_->b.commit(net::buffer_copy(
        in_->b.prepare(s.size()),
        net::buffer(s.data(), s.size())));
}

template<class Executor>
void
basic_stream<Executor>::
connect(basic_stream& remote)
{
    BOOST_ASSERT(! out_.lock());
    BOOST_ASSERT(! remote.out_.lock());
    std::lock(in_->m, remote.in_->m);
    std::lock_guard<std::mutex> guard1{in_->m, std::adopt_lock};
    std::lock_guard<std::mutex> guard2{remote.in_->m, std::adopt_lock};
    out_ = remote.in_;
    remote.out_ = in_;
    in_->code = detail::stream_status::ok;
    remote.in_->code = detail::stream_status::ok;
}

template<class Executor>
string_view
basic_stream<Executor>::
str() const
{
    auto const bs = in_->b.data();
    if(buffer_bytes(bs) == 0)
        return {};
    net::const_buffer const b = *net::buffer_sequence_begin(bs);
    return {static_cast<char const*>(b.data()), b.size()};
}

template<class Executor>
void
basic_stream<Executor>::
append(string_view s)
{
    std::lock_guard<std::mutex> lock{in_->m};
    in_->b.commit(net::buffer_copy(
        in_->b.prepare(s.size()),
        net::buffer(s.data(), s.size())));
}

template<class Executor>
void
basic_stream<Executor>::
clear()
{
    std::lock_guard<std::mutex> lock{in_->m};
    in_->b.consume(in_->b.size());
}

template<class Executor>
void
basic_stream<Executor>::
close()
{
    in_->cancel_read();

    // disconnect
    {
        auto out = out_.lock();
        out_.reset();

        // notify peer
        if(out)
        {
            std::lock_guard<std::mutex> lock(out->m);
            if(out->code == detail::stream_status::ok)
            {
                out->code = detail::stream_status::eof;
                out->notify_read();
            }
        }
    }
}

template<class Executor>
void
basic_stream<Executor>::
close_remote()
{
    std::lock_guard<std::mutex> lock{in_->m};
    if(in_->code == detail::stream_status::ok)
    {
        in_->code = detail::stream_status::eof;
        in_->notify_read();
    }
}

template<class Executor>
void
teardown(
    role_type,
    basic_stream<Executor>& s,
    boost::system::error_code& ec)
{
    if( s.in_->fc &&
        s.in_->fc->fail(ec))
        return;

    s.close();

    if( s.in_->fc &&
        s.in_->fc->fail(ec))
        ec = net::error::eof;
    else
        ec = {};
}

//------------------------------------------------------------------------------

template<class Executor>
basic_stream<Executor>
connect(basic_stream<Executor>& to)
{
    basic_stream<Executor> from(to.get_executor());
    from.connect(to);
    return from;
}

template<class Executor>
void
connect(basic_stream<Executor>& s1, basic_stream<Executor>& s2)
{
    s1.connect(s2);
}

} // test
} // beast
} // boost

#endif
