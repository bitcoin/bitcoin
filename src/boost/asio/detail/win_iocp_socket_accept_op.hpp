//
// detail/win_iocp_socket_accept_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_ACCEPT_OP_HPP
#define BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_ACCEPT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_IOCP)

#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/handler_work.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/operation.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/win_iocp_socket_service_base.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Socket, typename Protocol,
    typename Handler, typename IoExecutor>
class win_iocp_socket_accept_op : public operation
{
public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(win_iocp_socket_accept_op);

  win_iocp_socket_accept_op(win_iocp_socket_service_base& socket_service,
      socket_type socket, Socket& peer, const Protocol& protocol,
      typename Protocol::endpoint* peer_endpoint,
      bool enable_connection_aborted, Handler& handler, const IoExecutor& io_ex)
    : operation(&win_iocp_socket_accept_op::do_complete),
      socket_service_(socket_service),
      socket_(socket),
      peer_(peer),
      protocol_(protocol),
      peer_endpoint_(peer_endpoint),
      enable_connection_aborted_(enable_connection_aborted),
      cancel_requested_(0),
      proxy_op_(0),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      work_(handler_, io_ex)
  {
  }

  socket_holder& new_socket()
  {
    return new_socket_;
  }

  void* output_buffer()
  {
    return output_buffer_;
  }

  DWORD address_length()
  {
    return sizeof(sockaddr_storage_type) + 16;
  }

  void enable_cancellation(long* cancel_requested, operation* proxy_op)
  {
    cancel_requested_ = cancel_requested;
    proxy_op_ = proxy_op;
  }

  static void do_complete(void* owner, operation* base,
      const boost::system::error_code& result_ec,
      std::size_t /*bytes_transferred*/)
  {
    boost::system::error_code ec(result_ec);

    // Take ownership of the operation object.
    win_iocp_socket_accept_op* o(static_cast<win_iocp_socket_accept_op*>(base));
    ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

    if (owner)
    {
      typename Protocol::endpoint peer_endpoint;
      std::size_t addr_len = peer_endpoint.capacity();
      socket_ops::complete_iocp_accept(o->socket_,
          o->output_buffer(), o->address_length(),
          peer_endpoint.data(), &addr_len,
          o->new_socket_.get(), ec);

      // Restart the accept operation if we got the connection_aborted error
      // and the enable_connection_aborted socket option is not set.
      if (ec == boost::asio::error::connection_aborted
          && !o->enable_connection_aborted_)
      {
        o->reset();
        if (o->proxy_op_)
          o->proxy_op_->reset();
        o->socket_service_.restart_accept_op(o->socket_,
            o->new_socket_, o->protocol_.family(),
            o->protocol_.type(), o->protocol_.protocol(),
            o->output_buffer(), o->address_length(),
            o->cancel_requested_, o->proxy_op_ ? o->proxy_op_ : o);
        p.v = p.p = 0;
        return;
      }

      // If the socket was successfully accepted, transfer ownership of the
      // socket to the peer object.
      if (!ec)
      {
        o->peer_.assign(o->protocol_,
            typename Socket::native_handle_type(
              o->new_socket_.get(), peer_endpoint), ec);
        if (!ec)
          o->new_socket_.release();
      }

      // Pass endpoint back to caller.
      if (o->peer_endpoint_)
        *o->peer_endpoint_ = peer_endpoint;
    }

    BOOST_ASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        BOOST_ASIO_MOVE_CAST2(handler_work<Handler, IoExecutor>)(
          o->work_));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::binder1<Handler, boost::system::error_code>
      handler(o->handler_, ec);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      w.complete(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  win_iocp_socket_service_base& socket_service_;
  socket_type socket_;
  socket_holder new_socket_;
  Socket& peer_;
  Protocol protocol_;
  typename Protocol::endpoint* peer_endpoint_;
  unsigned char output_buffer_[(sizeof(sockaddr_storage_type) + 16) * 2];
  bool enable_connection_aborted_;
  operation* proxy_op_;
  long* cancel_requested_;
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

#if defined(BOOST_ASIO_HAS_MOVE)

template <typename Protocol, typename PeerIoExecutor,
    typename Handler, typename IoExecutor>
class win_iocp_socket_move_accept_op : public operation
{
public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(win_iocp_socket_move_accept_op);

  win_iocp_socket_move_accept_op(
      win_iocp_socket_service_base& socket_service, socket_type socket,
      const Protocol& protocol, const PeerIoExecutor& peer_io_ex,
      typename Protocol::endpoint* peer_endpoint,
      bool enable_connection_aborted, Handler& handler, const IoExecutor& io_ex)
    : operation(&win_iocp_socket_move_accept_op::do_complete),
      socket_service_(socket_service),
      socket_(socket),
      peer_(peer_io_ex),
      protocol_(protocol),
      peer_endpoint_(peer_endpoint),
      enable_connection_aborted_(enable_connection_aborted),
      cancel_requested_(0),
      proxy_op_(0),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler)),
      work_(handler_, io_ex)
  {
  }

  socket_holder& new_socket()
  {
    return new_socket_;
  }

  void* output_buffer()
  {
    return output_buffer_;
  }

  DWORD address_length()
  {
    return sizeof(sockaddr_storage_type) + 16;
  }

  void enable_cancellation(long* cancel_requested, operation* proxy_op)
  {
    cancel_requested_ = cancel_requested;
    proxy_op_ = proxy_op;
  }

  static void do_complete(void* owner, operation* base,
      const boost::system::error_code& result_ec,
      std::size_t /*bytes_transferred*/)
  {
    boost::system::error_code ec(result_ec);

    // Take ownership of the operation object.
    win_iocp_socket_move_accept_op* o(
        static_cast<win_iocp_socket_move_accept_op*>(base));
    ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

    if (owner)
    {
      typename Protocol::endpoint peer_endpoint;
      std::size_t addr_len = peer_endpoint.capacity();
      socket_ops::complete_iocp_accept(o->socket_,
          o->output_buffer(), o->address_length(),
          peer_endpoint.data(), &addr_len,
          o->new_socket_.get(), ec);

      // Restart the accept operation if we got the connection_aborted error
      // and the enable_connection_aborted socket option is not set.
      if (ec == boost::asio::error::connection_aborted
          && !o->enable_connection_aborted_)
      {
        o->reset();
        if (o->proxy_op_)
          o->proxy_op_->reset();
        o->socket_service_.restart_accept_op(o->socket_,
            o->new_socket_, o->protocol_.family(),
            o->protocol_.type(), o->protocol_.protocol(),
            o->output_buffer(), o->address_length(),
            o->cancel_requested_, o->proxy_op_ ? o->proxy_op_ : o);
        p.v = p.p = 0;
        return;
      }

      // If the socket was successfully accepted, transfer ownership of the
      // socket to the peer object.
      if (!ec)
      {
        o->peer_.assign(o->protocol_,
            typename Protocol::socket::native_handle_type(
              o->new_socket_.get(), peer_endpoint), ec);
        if (!ec)
          o->new_socket_.release();
      }

      // Pass endpoint back to caller.
      if (o->peer_endpoint_)
        *o->peer_endpoint_ = peer_endpoint;
    }

    BOOST_ASIO_HANDLER_COMPLETION((*o));

    // Take ownership of the operation's outstanding work.
    handler_work<Handler, IoExecutor> w(
        BOOST_ASIO_MOVE_CAST2(handler_work<Handler, IoExecutor>)(
          o->work_));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    detail::move_binder2<Handler,
      boost::system::error_code, peer_socket_type>
        handler(0, BOOST_ASIO_MOVE_CAST(Handler)(o->handler_), ec,
          BOOST_ASIO_MOVE_CAST(peer_socket_type)(o->peer_));
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      fenced_block b(fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, "..."));
      w.complete(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  typedef typename Protocol::socket::template
    rebind_executor<PeerIoExecutor>::other peer_socket_type;

  win_iocp_socket_service_base& socket_service_;
  socket_type socket_;
  socket_holder new_socket_;
  peer_socket_type peer_;
  Protocol protocol_;
  typename Protocol::endpoint* peer_endpoint_;
  unsigned char output_buffer_[(sizeof(sockaddr_storage_type) + 16) * 2];
  bool enable_connection_aborted_;
  long* cancel_requested_;
  operation* proxy_op_;
  Handler handler_;
  handler_work<Handler, IoExecutor> work_;
};

#endif // defined(BOOST_ASIO_HAS_MOVE)

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_ACCEPT_OP_HPP
