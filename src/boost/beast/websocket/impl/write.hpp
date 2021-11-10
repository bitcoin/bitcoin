//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_WRITE_HPP
#define BOOST_BEAST_WEBSOCKET_IMPL_WRITE_HPP

#include <boost/beast/websocket/detail/mask.hpp>
#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_cat.hpp>
#include <boost/beast/core/buffers_prefix.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/beast/core/buffers_suffix.hpp>
#include <boost/beast/core/flat_static_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/detail/bind_continuation.hpp>
#include <boost/beast/core/detail/clamp.hpp>
#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/websocket/detail/frame.hpp>
#include <boost/beast/websocket/impl/stream_impl.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <memory>

namespace boost {
namespace beast {
namespace websocket {

template<class NextLayer, bool deflateSupported>
template<class Handler, class Buffers>
class stream<NextLayer, deflateSupported>::write_some_op
    : public beast::async_base<
        Handler, beast::executor_type<stream>>
    , public asio::coroutine
{
    enum
    {
        do_nomask_nofrag,
        do_nomask_frag,
        do_mask_nofrag,
        do_mask_frag,
        do_deflate
    };

    boost::weak_ptr<impl_type> wp_;
    buffers_suffix<Buffers> cb_;
    detail::frame_header fh_;
    detail::prepared_key key_;
    std::size_t bytes_transferred_ = 0;
    std::size_t remain_;
    std::size_t in_;
    int how_;
    bool fin_;
    bool more_ = false; // for ubsan
    bool cont_ = false;

public:
    static constexpr int id = 2; // for soft_mutex

    template<class Handler_>
    write_some_op(
        Handler_&& h,
        boost::shared_ptr<impl_type> const& sp,
        bool fin,
        Buffers const& bs)
        : beast::async_base<Handler,
            beast::executor_type<stream>>(
                std::forward<Handler_>(h),
                    sp->stream().get_executor())
        , wp_(sp)
        , cb_(bs)
        , fin_(fin)
    {
        auto& impl = *sp;

        // Set up the outgoing frame header
        if(! impl.wr_cont)
        {
            impl.begin_msg();
            fh_.rsv1 = impl.wr_compress;
        }
        else
        {
            fh_.rsv1 = false;
        }
        fh_.rsv2 = false;
        fh_.rsv3 = false;
        fh_.op = impl.wr_cont ?
            detail::opcode::cont : impl.wr_opcode;
        fh_.mask =
            impl.role == role_type::client;

        // Choose a write algorithm
        if(impl.wr_compress)
        {
            how_ = do_deflate;
        }
        else if(! fh_.mask)
        {
            if(! impl.wr_frag)
            {
                how_ = do_nomask_nofrag;
            }
            else
            {
                BOOST_ASSERT(impl.wr_buf_size != 0);
                remain_ = buffer_bytes(cb_);
                if(remain_ > impl.wr_buf_size)
                    how_ = do_nomask_frag;
                else
                    how_ = do_nomask_nofrag;
            }
        }
        else
        {
            if(! impl.wr_frag)
            {
                how_ = do_mask_nofrag;
            }
            else
            {
                BOOST_ASSERT(impl.wr_buf_size != 0);
                remain_ = buffer_bytes(cb_);
                if(remain_ > impl.wr_buf_size)
                    how_ = do_mask_frag;
                else
                    how_ = do_mask_nofrag;
            }
        }
        (*this)({}, 0, false);
    }

    void operator()(
        error_code ec = {},
        std::size_t bytes_transferred = 0,
        bool cont = true);
};

template<class NextLayer, bool deflateSupported>
template<class Handler, class Buffers>
void
stream<NextLayer, deflateSupported>::
write_some_op<Handler, Buffers>::
operator()(
    error_code ec,
    std::size_t bytes_transferred,
    bool cont)
{
    using beast::detail::clamp;
    std::size_t n;
    net::mutable_buffer b;
    auto sp = wp_.lock();
    if(! sp)
    {
        ec = net::error::operation_aborted;
        bytes_transferred_ = 0;
        return this->complete(cont, ec, bytes_transferred_);
    }
    auto& impl = *sp;
    BOOST_ASIO_CORO_REENTER(*this)
    {
        // Acquire the write lock
        if(! impl.wr_block.try_lock(this))
        {
        do_suspend:
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    fin_ ?
                        "websocket::async_write" :
                        "websocket::async_write_some"
                    ));

                impl.op_wr.emplace(std::move(*this));
            }
            impl.wr_block.lock(this);
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    fin_ ?
                        "websocket::async_write" :
                        "websocket::async_write_some"
                    ));

                net::post(std::move(*this));
            }
            BOOST_ASSERT(impl.wr_block.is_locked(this));
        }
        if(impl.check_stop_now(ec))
            goto upcall;

        //------------------------------------------------------------------

        if(how_ == do_nomask_nofrag)
        {
            // send a single frame
            fh_.fin = fin_;
            fh_.len = buffer_bytes(cb_);
            impl.wr_fb.clear();
            detail::write<flat_static_buffer_base>(
                impl.wr_fb, fh_);
            impl.wr_cont = ! fin_;
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    fin_ ?
                        "websocket::async_write" :
                        "websocket::async_write_some"
                    ));

                net::async_write(impl.stream(),
                    buffers_cat(
                        net::const_buffer(impl.wr_fb.data()),
                        net::const_buffer(0, 0),
                        cb_,
                        buffers_prefix(0, cb_)
                        ),
                        beast::detail::bind_continuation(std::move(*this)));
            }
            bytes_transferred_ += clamp(fh_.len);
            if(impl.check_stop_now(ec))
                goto upcall;
            goto upcall;
        }

        //------------------------------------------------------------------

        if(how_ == do_nomask_frag)
        {
            // send multiple frames
            for(;;)
            {
                n = clamp(remain_, impl.wr_buf_size);
                fh_.len = n;
                remain_ -= n;
                fh_.fin = fin_ ? remain_ == 0 : false;
                impl.wr_fb.clear();
                detail::write<flat_static_buffer_base>(
                    impl.wr_fb, fh_);
                impl.wr_cont = ! fin_;
                // Send frame
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        fin_ ?
                            "websocket::async_write" :
                            "websocket::async_write_some"
                        ));

                    buffers_suffix<Buffers> empty_cb(cb_);
                    empty_cb.consume(~std::size_t(0));

                    net::async_write(impl.stream(),
                        buffers_cat(
                            net::const_buffer(impl.wr_fb.data()),
                            net::const_buffer(0, 0),
                            empty_cb,
                            buffers_prefix(clamp(fh_.len), cb_)
                            ),
                            beast::detail::bind_continuation(std::move(*this)));
                }
                n = clamp(fh_.len); // restore `n` on yield
                bytes_transferred_ += n;
                if(impl.check_stop_now(ec))
                    goto upcall;
                if(remain_ == 0)
                    break;
                cb_.consume(n);
                fh_.op = detail::opcode::cont;

                // Give up the write lock in between each frame
                // so that outgoing control frames might be sent.
                impl.wr_block.unlock(this);
                if( impl.op_close.maybe_invoke()
                    || impl.op_idle_ping.maybe_invoke()
                    || impl.op_rd.maybe_invoke()
                    || impl.op_ping.maybe_invoke())
                {
                    BOOST_ASSERT(impl.wr_block.is_locked());
                    goto do_suspend;
                }
                impl.wr_block.lock(this);
            }
            goto upcall;
        }

        //------------------------------------------------------------------

        if(how_ == do_mask_nofrag)
        {
            // send a single frame using multiple writes
            remain_ = beast::buffer_bytes(cb_);
            fh_.fin = fin_;
            fh_.len = remain_;
            fh_.key = impl.create_mask();
            detail::prepare_key(key_, fh_.key);
            impl.wr_fb.clear();
            detail::write<flat_static_buffer_base>(
                impl.wr_fb, fh_);
            n = clamp(remain_, impl.wr_buf_size);
            net::buffer_copy(net::buffer(
                impl.wr_buf.get(), n), cb_);
            detail::mask_inplace(net::buffer(
                impl.wr_buf.get(), n), key_);
            remain_ -= n;
            impl.wr_cont = ! fin_;
            // write frame header and some payload
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    fin_ ?
                        "websocket::async_write" :
                        "websocket::async_write_some"
                    ));

                buffers_suffix<Buffers> empty_cb(cb_);
                empty_cb.consume(~std::size_t(0));

                net::async_write(impl.stream(),
                    buffers_cat(
                        net::const_buffer(impl.wr_fb.data()),
                        net::const_buffer(net::buffer(impl.wr_buf.get(), n)),
                        empty_cb,
                        buffers_prefix(0, empty_cb)
                        ),
                        beast::detail::bind_continuation(std::move(*this)));
            }
            // VFALCO What about consuming the buffer on error?
            bytes_transferred_ +=
                bytes_transferred - impl.wr_fb.size();
            if(impl.check_stop_now(ec))
                goto upcall;
            while(remain_ > 0)
            {
                cb_.consume(impl.wr_buf_size);
                n = clamp(remain_, impl.wr_buf_size);
                net::buffer_copy(net::buffer(
                    impl.wr_buf.get(), n), cb_);
                detail::mask_inplace(net::buffer(
                    impl.wr_buf.get(), n), key_);
                remain_ -= n;
                // write more payload
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        fin_ ?
                            "websocket::async_write" :
                            "websocket::async_write_some"
                        ));

                    buffers_suffix<Buffers> empty_cb(cb_);
                    empty_cb.consume(~std::size_t(0));

                    net::async_write(impl.stream(),
                        buffers_cat(
                            net::const_buffer(0, 0),
                            net::const_buffer(net::buffer(impl.wr_buf.get(), n)),
                            empty_cb,
                            buffers_prefix(0, empty_cb)
                            ),
                            beast::detail::bind_continuation(std::move(*this)));
                }
                bytes_transferred_ += bytes_transferred;
                if(impl.check_stop_now(ec))
                    goto upcall;
            }
            goto upcall;
        }

        //------------------------------------------------------------------

        if(how_ == do_mask_frag)
        {
            // send multiple frames
            for(;;)
            {
                n = clamp(remain_, impl.wr_buf_size);
                remain_ -= n;
                fh_.len = n;
                fh_.key = impl.create_mask();
                fh_.fin = fin_ ? remain_ == 0 : false;
                detail::prepare_key(key_, fh_.key);
                net::buffer_copy(net::buffer(
                    impl.wr_buf.get(), n), cb_);
                detail::mask_inplace(net::buffer(
                    impl.wr_buf.get(), n), key_);
                impl.wr_fb.clear();
                detail::write<flat_static_buffer_base>(
                    impl.wr_fb, fh_);
                impl.wr_cont = ! fin_;
                // Send frame
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        fin_ ?
                            "websocket::async_write" :
                            "websocket::async_write_some"
                        ));

                    buffers_suffix<Buffers> empty_cb(cb_);
                    empty_cb.consume(~std::size_t(0));

                    net::async_write(impl.stream(),
                        buffers_cat(
                            net::const_buffer(impl.wr_fb.data()),
                            net::const_buffer(net::buffer(impl.wr_buf.get(), n)),
                            empty_cb,
                            buffers_prefix(0, empty_cb)
                            ),
                            beast::detail::bind_continuation(std::move(*this)));
                }
                n = bytes_transferred - impl.wr_fb.size();
                bytes_transferred_ += n;
                if(impl.check_stop_now(ec))
                    goto upcall;
                if(remain_ == 0)
                    break;
                cb_.consume(n);
                fh_.op = detail::opcode::cont;
                // Give up the write lock in between each frame
                // so that outgoing control frames might be sent.
                impl.wr_block.unlock(this);
                if( impl.op_close.maybe_invoke()
                    || impl.op_idle_ping.maybe_invoke()
                    || impl.op_rd.maybe_invoke()
                    || impl.op_ping.maybe_invoke())
                {
                    BOOST_ASSERT(impl.wr_block.is_locked());
                    goto do_suspend;
                }
                impl.wr_block.lock(this);
            }
            goto upcall;
        }

        //------------------------------------------------------------------

        if(how_ == do_deflate)
        {
            // send compressed frames
            for(;;)
            {
                b = net::buffer(impl.wr_buf.get(),
                    impl.wr_buf_size);
                more_ = impl.deflate(b, cb_, fin_, in_, ec);
                if(impl.check_stop_now(ec))
                    goto upcall;
                n = buffer_bytes(b);
                if(n == 0)
                {
                    // The input was consumed, but there is
                    // no output due to compression latency.
                    BOOST_ASSERT(! fin_);
                    BOOST_ASSERT(buffer_bytes(cb_) == 0);
                    goto upcall;
                }
                if(fh_.mask)
                {
                    fh_.key = impl.create_mask();
                    detail::prepared_key key;
                    detail::prepare_key(key, fh_.key);
                    detail::mask_inplace(b, key);
                }
                fh_.fin = ! more_;
                fh_.len = n;
                impl.wr_fb.clear();
                detail::write<
                    flat_static_buffer_base>(impl.wr_fb, fh_);
                impl.wr_cont = ! fin_;
                // Send frame
                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        fin_ ?
                            "websocket::async_write" :
                            "websocket::async_write_some"
                        ));

                    buffers_suffix<Buffers> empty_cb(cb_);
                    empty_cb.consume(~std::size_t(0));

                    net::async_write(impl.stream(),
                        buffers_cat(
                            net::const_buffer(impl.wr_fb.data()),
                            net::const_buffer(b),
                            empty_cb,
                            buffers_prefix(0, empty_cb)
                            ),
                            beast::detail::bind_continuation(std::move(*this)));
                }
                bytes_transferred_ += in_;
                if(impl.check_stop_now(ec))
                    goto upcall;
                if(more_)
                {
                    fh_.op = detail::opcode::cont;
                    fh_.rsv1 = false;
                    // Give up the write lock in between each frame
                    // so that outgoing control frames might be sent.
                    impl.wr_block.unlock(this);
                    if( impl.op_close.maybe_invoke()
                        || impl.op_idle_ping.maybe_invoke()
                        || impl.op_rd.maybe_invoke()
                        || impl.op_ping.maybe_invoke())
                    {
                        BOOST_ASSERT(impl.wr_block.is_locked());
                        goto do_suspend;
                    }
                    impl.wr_block.lock(this);
                }
                else
                {
                    if(fh_.fin)
                        impl.do_context_takeover_write(impl.role);
                    goto upcall;
                }
            }
        }

    //--------------------------------------------------------------------------

    upcall:
        impl.wr_block.unlock(this);
        impl.op_close.maybe_invoke()
            || impl.op_idle_ping.maybe_invoke()
            || impl.op_rd.maybe_invoke()
            || impl.op_ping.maybe_invoke();
        this->complete(cont, ec, bytes_transferred_);
    }
}

template<class NextLayer, bool deflateSupported>
struct stream<NextLayer, deflateSupported>::
    run_write_some_op
{
    template<
        class WriteHandler,
        class ConstBufferSequence>
    void
    operator()(
        WriteHandler&& h,
        boost::shared_ptr<impl_type> const& sp,
        bool fin,
        ConstBufferSequence const& b)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<WriteHandler,
                void(error_code, std::size_t)>::value,
            "WriteHandler type requirements not met");

        write_some_op<
            typename std::decay<WriteHandler>::type,
            ConstBufferSequence>(
                std::forward<WriteHandler>(h),
                sp,
                fin,
                b);
    }
};

//------------------------------------------------------------------------------

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence>
std::size_t
stream<NextLayer, deflateSupported>::
write_some(bool fin, ConstBufferSequence const& buffers)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    error_code ec;
    auto const bytes_transferred =
        write_some(fin, buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence>
std::size_t
stream<NextLayer, deflateSupported>::
write_some(bool fin,
    ConstBufferSequence const& buffers, error_code& ec)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    using beast::detail::clamp;
    auto& impl = *impl_;
    std::size_t bytes_transferred = 0;
    ec = {};
    if(impl.check_stop_now(ec))
        return bytes_transferred;
    detail::frame_header fh;
    if(! impl.wr_cont)
    {
        impl.begin_msg();
        fh.rsv1 = impl.wr_compress;
    }
    else
    {
        fh.rsv1 = false;
    }
    fh.rsv2 = false;
    fh.rsv3 = false;
    fh.op = impl.wr_cont ?
        detail::opcode::cont : impl.wr_opcode;
    fh.mask = impl.role == role_type::client;
    auto remain = buffer_bytes(buffers);
    if(impl.wr_compress)
    {

        buffers_suffix<
            ConstBufferSequence> cb(buffers);
        for(;;)
        {
            auto b = net::buffer(
                impl.wr_buf.get(), impl.wr_buf_size);
            auto const more = impl.deflate(
                b, cb, fin, bytes_transferred, ec);
            if(impl.check_stop_now(ec))
                return bytes_transferred;
            auto const n = buffer_bytes(b);
            if(n == 0)
            {
                // The input was consumed, but there
                // is no output due to compression
                // latency.
                BOOST_ASSERT(! fin);
                BOOST_ASSERT(buffer_bytes(cb) == 0);
                fh.fin = false;
                break;
            }
            if(fh.mask)
            {
                fh.key = this->impl_->create_mask();
                detail::prepared_key key;
                detail::prepare_key(key, fh.key);
                detail::mask_inplace(b, key);
            }
            fh.fin = ! more;
            fh.len = n;
            detail::fh_buffer fh_buf;
            detail::write<
                flat_static_buffer_base>(fh_buf, fh);
            impl.wr_cont = ! fin;
            net::write(impl.stream(),
                buffers_cat(fh_buf.data(), b), ec);
            if(impl.check_stop_now(ec))
                return bytes_transferred;
            if(! more)
                break;
            fh.op = detail::opcode::cont;
            fh.rsv1 = false;
        }
        if(fh.fin)
            impl.do_context_takeover_write(impl.role);
    }
    else if(! fh.mask)
    {
        if(! impl.wr_frag)
        {
            // no mask, no autofrag
            fh.fin = fin;
            fh.len = remain;
            detail::fh_buffer fh_buf;
            detail::write<
                flat_static_buffer_base>(fh_buf, fh);
            impl.wr_cont = ! fin;
            net::write(impl.stream(),
                buffers_cat(fh_buf.data(), buffers), ec);
            if(impl.check_stop_now(ec))
                return bytes_transferred;
            bytes_transferred += remain;
        }
        else
        {
            // no mask, autofrag
            BOOST_ASSERT(impl.wr_buf_size != 0);
            buffers_suffix<
                ConstBufferSequence> cb{buffers};
            for(;;)
            {
                auto const n = clamp(remain, impl.wr_buf_size);
                remain -= n;
                fh.len = n;
                fh.fin = fin ? remain == 0 : false;
                detail::fh_buffer fh_buf;
                detail::write<
                    flat_static_buffer_base>(fh_buf, fh);
                impl.wr_cont = ! fin;
                net::write(impl.stream(),
                    beast::buffers_cat(fh_buf.data(),
                        beast::buffers_prefix(n, cb)), ec);
                bytes_transferred += n;
                if(impl.check_stop_now(ec))
                    return bytes_transferred;
                if(remain == 0)
                    break;
                fh.op = detail::opcode::cont;
                cb.consume(n);
            }
        }
    }
    else if(! impl.wr_frag)
    {
        // mask, no autofrag
        fh.fin = fin;
        fh.len = remain;
        fh.key = this->impl_->create_mask();
        detail::prepared_key key;
        detail::prepare_key(key, fh.key);
        detail::fh_buffer fh_buf;
        detail::write<
            flat_static_buffer_base>(fh_buf, fh);
        buffers_suffix<
            ConstBufferSequence> cb{buffers};
        {
            auto const n =
                clamp(remain, impl.wr_buf_size);
            auto const b =
                net::buffer(impl.wr_buf.get(), n);
            net::buffer_copy(b, cb);
            cb.consume(n);
            remain -= n;
            detail::mask_inplace(b, key);
            impl.wr_cont = ! fin;
            net::write(impl.stream(),
                buffers_cat(fh_buf.data(), b), ec);
            bytes_transferred += n;
            if(impl.check_stop_now(ec))
                return bytes_transferred;
        }
        while(remain > 0)
        {
            auto const n =
                clamp(remain, impl.wr_buf_size);
            auto const b =
                net::buffer(impl.wr_buf.get(), n);
            net::buffer_copy(b, cb);
            cb.consume(n);
            remain -= n;
            detail::mask_inplace(b, key);
            net::write(impl.stream(), b, ec);
            bytes_transferred += n;
            if(impl.check_stop_now(ec))
                return bytes_transferred;
        }
    }
    else
    {
        // mask, autofrag
        BOOST_ASSERT(impl.wr_buf_size != 0);
        buffers_suffix<
            ConstBufferSequence> cb(buffers);
        for(;;)
        {
            fh.key = this->impl_->create_mask();
            detail::prepared_key key;
            detail::prepare_key(key, fh.key);
            auto const n =
                clamp(remain, impl.wr_buf_size);
            auto const b =
                net::buffer(impl.wr_buf.get(), n);
            net::buffer_copy(b, cb);
            detail::mask_inplace(b, key);
            fh.len = n;
            remain -= n;
            fh.fin = fin ? remain == 0 : false;
            impl.wr_cont = ! fh.fin;
            detail::fh_buffer fh_buf;
            detail::write<
                flat_static_buffer_base>(fh_buf, fh);
            net::write(impl.stream(),
                buffers_cat(fh_buf.data(), b), ec);
            bytes_transferred += n;
            if(impl.check_stop_now(ec))
                return bytes_transferred;
            if(remain == 0)
                break;
            fh.op = detail::opcode::cont;
            cb.consume(n);
        }
    }
    return bytes_transferred;
}

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
stream<NextLayer, deflateSupported>::
async_write_some(bool fin,
    ConstBufferSequence const& bs, WriteHandler&& handler)
{
    static_assert(is_async_stream<next_layer_type>::value,
        "AsyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    return net::async_initiate<
        WriteHandler,
        void(error_code, std::size_t)>(
            run_write_some_op{},
            handler,
            impl_,
            fin,
            bs);
}

//------------------------------------------------------------------------------

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence>
std::size_t
stream<NextLayer, deflateSupported>::
write(ConstBufferSequence const& buffers)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    error_code ec;
    auto const bytes_transferred = write(buffers, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
    return bytes_transferred;
}

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence>
std::size_t
stream<NextLayer, deflateSupported>::
write(ConstBufferSequence const& buffers, error_code& ec)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    return write_some(true, buffers, ec);
}

template<class NextLayer, bool deflateSupported>
template<class ConstBufferSequence, BOOST_BEAST_ASYNC_TPARAM2 WriteHandler>
BOOST_BEAST_ASYNC_RESULT2(WriteHandler)
stream<NextLayer, deflateSupported>::
async_write(
    ConstBufferSequence const& bs, WriteHandler&& handler)
{
    static_assert(is_async_stream<next_layer_type>::value,
        "AsyncStream type requirements not met");
    static_assert(net::is_const_buffer_sequence<
        ConstBufferSequence>::value,
            "ConstBufferSequence type requirements not met");
    return net::async_initiate<
        WriteHandler,
        void(error_code, std::size_t)>(
            run_write_some_op{},
            handler,
            impl_,
            true,
            bs);
}

} // websocket
} // beast
} // boost

#endif
