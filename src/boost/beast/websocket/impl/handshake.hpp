//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_IMPL_HANDSHAKE_HPP
#define BOOST_BEAST_WEBSOCKET_IMPL_HANDSHAKE_HPP

#include <boost/beast/websocket/impl/stream_impl.hpp>
#include <boost/beast/websocket/detail/type_traits.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/beast/core/async_base.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <memory>

namespace boost {
namespace beast {
namespace websocket {

//------------------------------------------------------------------------------

// send the upgrade request and process the response
//
template<class NextLayer, bool deflateSupported>
template<class Handler>
class stream<NextLayer, deflateSupported>::handshake_op
    : public beast::stable_async_base<Handler,
        beast::executor_type<stream>>
    , public asio::coroutine
{
    struct data
    {
        // VFALCO This really should be two separate
        //        composed operations, to save on memory
        request_type req;
        http::response_parser<
            typename response_type::body_type> p;
        flat_buffer fb;
        bool overflow = false; // could be a member of the op

        explicit
        data(request_type&& req_)
            : req(std::move(req_))
        {
        }
    };

    boost::weak_ptr<impl_type> wp_;
    detail::sec_ws_key_type key_;
    response_type* res_p_;
    data& d_;

public:
    template<class Handler_>
    handshake_op(
        Handler_&& h,
        boost::shared_ptr<impl_type> const& sp,
        request_type&& req,
        detail::sec_ws_key_type key,
        response_type* res_p)
        : stable_async_base<Handler,
            beast::executor_type<stream>>(
                std::forward<Handler_>(h),
                    sp->stream().get_executor())
        , wp_(sp)
        , key_(key)
        , res_p_(res_p)
        , d_(beast::allocate_stable<data>(
            *this, std::move(req)))
    {
        sp->reset(); // VFALCO I don't like this
        (*this)({}, 0, false);
    }

    void
    operator()(
        error_code ec = {},
        std::size_t bytes_used = 0,
        bool cont = true)
    {
        boost::ignore_unused(bytes_used);
        auto sp = wp_.lock();
        if(! sp)
        {
            ec = net::error::operation_aborted;
            return this->complete(cont, ec);
        }
        auto& impl = *sp;
        BOOST_ASIO_CORO_REENTER(*this)
        {
            impl.change_status(status::handshake);
            impl.update_timer(this->get_executor());

            // write HTTP request
            impl.do_pmd_config(d_.req);
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    "websocket::async_handshake"));

                http::async_write(impl.stream(),
                    d_.req, std::move(*this));
            }
            if(impl.check_stop_now(ec))
                goto upcall;

            // read HTTP response
            BOOST_ASIO_CORO_YIELD
            {
                BOOST_ASIO_HANDLER_LOCATION((
                    __FILE__, __LINE__,
                    "websocket::async_handshake"));

                http::async_read(impl.stream(),
                    impl.rd_buf, d_.p,
                        std::move(*this));
            }
            if(ec == http::error::buffer_overflow)
            {
                // If the response overflows the internal
                // read buffer, switch to a dynamically
                // allocated flat buffer.

                d_.fb.commit(net::buffer_copy(
                    d_.fb.prepare(impl.rd_buf.size()),
                    impl.rd_buf.data()));
                impl.rd_buf.clear();

                BOOST_ASIO_CORO_YIELD
                {
                    BOOST_ASIO_HANDLER_LOCATION((
                        __FILE__, __LINE__,
                        "websocket::async_handshake"));

                    http::async_read(impl.stream(),
                        d_.fb, d_.p, std::move(*this));
                }

                if(! ec)
                {
                    // Copy any leftovers back into the read
                    // buffer, since this represents websocket
                    // frame data.

                    if(d_.fb.size() <= impl.rd_buf.capacity())
                    {
                        impl.rd_buf.commit(net::buffer_copy(
                            impl.rd_buf.prepare(d_.fb.size()),
                            d_.fb.data()));
                    }
                    else
                    {
                        ec = http::error::buffer_overflow;
                    }
                }

                // Do this before the upcall
                d_.fb.clear();
            }
            if(impl.check_stop_now(ec))
                goto upcall;

            // success
            impl.reset_idle();
            impl.on_response(d_.p.get(), key_, ec);
            if(res_p_)
                swap(d_.p.get(), *res_p_);

        upcall:
            this->complete(cont ,ec);
        }
    }
};

template<class NextLayer, bool deflateSupported>
struct stream<NextLayer, deflateSupported>::
    run_handshake_op
{
    template<class HandshakeHandler>
    void operator()(
        HandshakeHandler&& h,
        boost::shared_ptr<impl_type> const& sp,
        request_type&& req,
        detail::sec_ws_key_type key,
        response_type* res_p)
    {
        // If you get an error on the following line it means
        // that your handler does not meet the documented type
        // requirements for the handler.

        static_assert(
            beast::detail::is_invocable<HandshakeHandler,
                void(error_code)>::value,
            "HandshakeHandler type requirements not met");

        handshake_op<
            typename std::decay<HandshakeHandler>::type>(
                std::forward<HandshakeHandler>(h),
                    sp, std::move(req), key, res_p);
    }
};

//------------------------------------------------------------------------------

template<class NextLayer, bool deflateSupported>
template<class RequestDecorator>
void
stream<NextLayer, deflateSupported>::
do_handshake(
    response_type* res_p,
    string_view host,
    string_view target,
    RequestDecorator const& decorator,
    error_code& ec)
{
    auto& impl = *impl_;
    impl.change_status(status::handshake);
    impl.reset();
    detail::sec_ws_key_type key;
    {
        auto const req = impl.build_request(
            key, host, target, decorator);
        impl.do_pmd_config(req);
        http::write(impl.stream(), req, ec);
    }
    if(impl.check_stop_now(ec))
        return;
    http::response_parser<
        typename response_type::body_type> p;
    http::read(next_layer(), impl.rd_buf, p, ec);
    if(ec == http::error::buffer_overflow)
    {
        // If the response overflows the internal
        // read buffer, switch to a dynamically
        // allocated flat buffer.

        flat_buffer fb;
        fb.commit(net::buffer_copy(
            fb.prepare(impl.rd_buf.size()),
            impl.rd_buf.data()));
        impl.rd_buf.clear();

        http::read(next_layer(), fb, p, ec);;

        if(! ec)
        {
            // Copy any leftovers back into the read
            // buffer, since this represents websocket
            // frame data.

            if(fb.size() <= impl.rd_buf.capacity())
            {
                impl.rd_buf.commit(net::buffer_copy(
                    impl.rd_buf.prepare(fb.size()),
                    fb.data()));
            }
            else
            {
                ec = http::error::buffer_overflow;
            }
        }
    }
    if(impl.check_stop_now(ec))
        return;

    impl.on_response(p.get(), key, ec);
    if(impl.check_stop_now(ec))
        return;

    if(res_p)
        *res_p = p.release();
}

//------------------------------------------------------------------------------

template<class NextLayer, bool deflateSupported>
template<BOOST_BEAST_ASYNC_TPARAM1 HandshakeHandler>
BOOST_BEAST_ASYNC_RESULT1(HandshakeHandler)
stream<NextLayer, deflateSupported>::
async_handshake(
    string_view host,
    string_view target,
    HandshakeHandler&& handler)
{
    static_assert(is_async_stream<next_layer_type>::value,
        "AsyncStream type requirements not met");
    detail::sec_ws_key_type key;
    auto req = impl_->build_request(
        key, host, target, &default_decorate_req);
    return net::async_initiate<
        HandshakeHandler,
        void(error_code)>(
            run_handshake_op{},
            handler,
            impl_,
            std::move(req),
            key,
            nullptr);
}

template<class NextLayer, bool deflateSupported>
template<BOOST_BEAST_ASYNC_TPARAM1 HandshakeHandler>
BOOST_BEAST_ASYNC_RESULT1(HandshakeHandler)
stream<NextLayer, deflateSupported>::
async_handshake(
    response_type& res,
    string_view host,
    string_view target,
    HandshakeHandler&& handler)
{
    static_assert(is_async_stream<next_layer_type>::value,
        "AsyncStream type requirements not met");
    detail::sec_ws_key_type key;
    auto req = impl_->build_request(
        key, host, target, &default_decorate_req);
    return net::async_initiate<
        HandshakeHandler,
        void(error_code)>(
            run_handshake_op{},
            handler,
            impl_,
            std::move(req),
            key,
            &res);
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
handshake(string_view host,
    string_view target)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    error_code ec;
    handshake(
        host, target, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
handshake(response_type& res,
    string_view host,
        string_view target)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    error_code ec;
    handshake(res, host, target, ec);
    if(ec)
        BOOST_THROW_EXCEPTION(system_error{ec});
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
handshake(string_view host,
    string_view target, error_code& ec)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    do_handshake(nullptr,
        host, target, &default_decorate_req, ec);
}

template<class NextLayer, bool deflateSupported>
void
stream<NextLayer, deflateSupported>::
handshake(response_type& res,
    string_view host,
        string_view target,
            error_code& ec)
{
    static_assert(is_sync_stream<next_layer_type>::value,
        "SyncStream type requirements not met");
    do_handshake(&res,
        host, target, &default_decorate_req, ec);
}

} // websocket
} // beast
} // boost

#endif
