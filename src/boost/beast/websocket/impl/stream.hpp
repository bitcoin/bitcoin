//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_STREAM_HPP
#define BOOST_BEAST_WEBSOCKET_IMPL_STREAM_HPP

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/teardown.hpp>
#include <boost/beast/websocket/detail/hybi13.hpp>
#include <boost/beast/websocket/detail/mask.hpp>
#include <boost/beast/websocket/impl/stream_impl.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/http/rfc7230.hpp>
#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/core/buffers_suffix.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/assert.hpp>
#include <boost/make_shared.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>

namespace boost {
namespace beast {
namespace websocket {

template<class NextLayer, bool deflateSupported>
stream<NextLayer, deflateSupported>::
~stream()
{
    if(impl_)
        impl_->remove();
}

template<class NextLayer, bool deflateSupported>
template<class... Args>
stream<NextLayer, deflateSupported>::
stream(Args&&... args)
    : impl_(boost::make_shared<impl_type>(
        std::forward<Args>(args)...))
{
    BOOST_ASSERT(impl_->rd_buf.max_size() >=
        max_control_frame_size);
}

template<class NextLayer, bool deflateSupported>
auto
stream<NextLayer, deflateSupported>::
get_executor() noexcept ->
    executor_type
{
    return impl_->stream().get_executor();
}

template<class NextLayer, bool deflateSupported>
auto
stream<NextLayer, deflateSupported>::
next_layer() noexcept ->
    next_layer_type&
{
    return impl_->stream();
}

template<class NextLayer, bool deflateSupported>
auto
stream<NextLayer, deflateSupported>::
next_layer() const noexcept ->
    next_layer_type const&
{
    return impl_->stream();
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
is_open() const noexcept
{
    return impl_->status_ == status::open;
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
got_binary() const noexcept
{
    return impl_->rd_op == detail::opcode::binary;
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
is_message_done() const noexcept
{
    return impl_->rd_done;
}

template<class NextLayer, bool deflateSupported>
close_reason const&
stream<NextLayer, deflateSupported>::
reason() const noexcept
{
    return impl_->cr;
}

template<class NextLayer, bool deflateSupported>
std::size_t
stream<NextLayer, deflateSupported>::
read_size_hint(
    std::size_t initial_size) const
{
    return impl_->read_size_hint_pmd(
        initial_size, impl_->rd_done,
        impl_->rd_remain, impl_->rd_fh);
}

template<class NextLayer, bool deflateSupported>
template<class DynamicBuffer, class>
std::size_t
stream<NextLayer, deflateSupported>::
read_size_hint(DynamicBuffer& buffer) const
{
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");
    return impl_->read_size_hint_db(buffer);
}

//------------------------------------------------------------------------------
//
// Settings
//
//------------------------------------------------------------------------------

// decorator

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
set_option(decorator opt)
{
    impl_->decorator_opt = std::move(opt.d_);
}

// timeout

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
get_option(timeout& opt)
{
    opt = impl_->timeout_opt;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
set_option(timeout const& opt)
{
    impl_->set_option(opt);
}

//

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
set_option(permessage_deflate const& o)
{
    impl_->set_option_pmd(o);
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
get_option(permessage_deflate& o)
{
    impl_->get_option_pmd(o);
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
auto_fragment(bool value)
{
    impl_->wr_frag_opt = value;
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
auto_fragment() const
{
    return impl_->wr_frag_opt;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
binary(bool value)
{
    impl_->wr_opcode = value ?
        detail::opcode::binary :
        detail::opcode::text;
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
binary() const
{
    return impl_->wr_opcode == detail::opcode::binary;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
control_callback(std::function<
    void(frame_type, string_view)> cb)
{
    impl_->ctrl_cb = std::move(cb);
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
control_callback()
{
    impl_->ctrl_cb = {};
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
read_message_max(std::size_t amount)
{
    impl_->rd_msg_max = amount;
}

template<class NextLayer, bool deflateSupported>
std::size_t
stream<NextLayer, deflateSupported>::
read_message_max() const
{
    return impl_->rd_msg_max;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
secure_prng(bool value)
{
    this->impl_->secure_prng_ = value;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
write_buffer_bytes(std::size_t amount)
{
    if(amount < 8)
        BOOST_THROW_EXCEPTION(std::invalid_argument{
            "write buffer size underflow"});
    impl_->wr_buf_opt = amount;
}

template<class NextLayer, bool deflateSupported>
std::size_t
stream<NextLayer, deflateSupported>::
write_buffer_bytes() const
{
    return impl_->wr_buf_opt;
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
text(bool value)
{
    impl_->wr_opcode = value ?
        detail::opcode::text :
        detail::opcode::binary;
}

template<class NextLayer, bool deflateSupported>
bool
stream<NextLayer, deflateSupported>::
text() const
{
    return impl_->wr_opcode == detail::opcode::text;
}

//------------------------------------------------------------------------------

// _Fail the WebSocket Connection_
template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
do_fail(
    std::uint16_t code,         // if set, send a close frame first
    error_code ev,              // error code to use upon success
    error_code& ec)             // set to the error, else set to ev
{
    BOOST_ASSERT(ev);
    impl_->change_status(status::closing);
    if(code != close_code::none && ! impl_->wr_close)
    {
        impl_->wr_close = true;
        detail::frame_buffer fb;
        impl_->template write_close<
            flat_static_buffer_base>(fb, code);
        net::write(impl_->stream(), fb.data(), ec);
        if(impl_->check_stop_now(ec))
            return;
    }
    using beast::websocket::teardown;
    teardown(impl_->role, impl_->stream(), ec);
    if(ec == net::error::eof)
    {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec = {};
    }
    if(! ec)
        ec = ev;
    if(ec && ec != error::closed)
        impl_->change_status(status::failed);
    else
        impl_->change_status(status::closed);
    impl_->close();
}

} // websocket
} // beast
} // boost

#endif
