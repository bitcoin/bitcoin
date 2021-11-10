//
// detail/win_iocp_socket_service_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_BASE_HPP
#define BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_IOCP)

#include <boost/asio/associated_cancellation_slot.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/mutex.hpp>
#include <boost/asio/detail/operation.hpp>
#include <boost/asio/detail/reactor_op.hpp>
#include <boost/asio/detail/select_reactor.hpp>
#include <boost/asio/detail/socket_holder.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/socket_types.hpp>
#include <boost/asio/detail/win_iocp_io_context.hpp>
#include <boost/asio/detail/win_iocp_null_buffers_op.hpp>
#include <boost/asio/detail/win_iocp_socket_connect_op.hpp>
#include <boost/asio/detail/win_iocp_socket_send_op.hpp>
#include <boost/asio/detail/win_iocp_socket_recv_op.hpp>
#include <boost/asio/detail/win_iocp_socket_recvmsg_op.hpp>
#include <boost/asio/detail/win_iocp_wait_op.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class win_iocp_socket_service_base
{
public:
  // The implementation type of the socket.
  struct base_implementation_type
  {
    // The native socket representation.
    socket_type socket_;

    // The current state of the socket.
    socket_ops::state_type state_;

    // We use a shared pointer as a cancellation token here to work around the
    // broken Windows support for cancellation. MSDN says that when you call
    // closesocket any outstanding WSARecv or WSASend operations will complete
    // with the error ERROR_OPERATION_ABORTED. In practice they complete with
    // ERROR_NETNAME_DELETED, which means you can't tell the difference between
    // a local cancellation and the socket being hard-closed by the peer.
    socket_ops::shared_cancel_token_type cancel_token_;

    // Per-descriptor data used by the reactor.
    select_reactor::per_descriptor_data reactor_data_;

#if defined(BOOST_ASIO_ENABLE_CANCELIO)
    // The ID of the thread from which it is safe to cancel asynchronous
    // operations. 0 means no asynchronous operations have been started yet.
    // ~0 means asynchronous operations have been started from more than one
    // thread, and cancellation is not supported for the socket.
    DWORD safe_cancellation_thread_id_;
#endif // defined(BOOST_ASIO_ENABLE_CANCELIO)

    // Pointers to adjacent socket implementations in linked list.
    base_implementation_type* next_;
    base_implementation_type* prev_;
  };

  // Constructor.
  BOOST_ASIO_DECL win_iocp_socket_service_base(execution_context& context);

  // Destroy all user-defined handler objects owned by the service.
  BOOST_ASIO_DECL void base_shutdown();

  // Construct a new socket implementation.
  BOOST_ASIO_DECL void construct(base_implementation_type& impl);

  // Move-construct a new socket implementation.
  BOOST_ASIO_DECL void base_move_construct(base_implementation_type& impl,
      base_implementation_type& other_impl) BOOST_ASIO_NOEXCEPT;

  // Move-assign from another socket implementation.
  BOOST_ASIO_DECL void base_move_assign(base_implementation_type& impl,
      win_iocp_socket_service_base& other_service,
      base_implementation_type& other_impl);

  // Destroy a socket implementation.
  BOOST_ASIO_DECL void destroy(base_implementation_type& impl);

  // Determine whether the socket is open.
  bool is_open(const base_implementation_type& impl) const
  {
    return impl.socket_ != invalid_socket;
  }

  // Destroy a socket implementation.
  BOOST_ASIO_DECL boost::system::error_code close(
      base_implementation_type& impl, boost::system::error_code& ec);

  // Release ownership of the socket.
  BOOST_ASIO_DECL socket_type release(
      base_implementation_type& impl, boost::system::error_code& ec);

  // Cancel all operations associated with the socket.
  BOOST_ASIO_DECL boost::system::error_code cancel(
      base_implementation_type& impl, boost::system::error_code& ec);

  // Determine whether the socket is at the out-of-band data mark.
  bool at_mark(const base_implementation_type& impl,
      boost::system::error_code& ec) const
  {
    return socket_ops::sockatmark(impl.socket_, ec);
  }

  // Determine the number of bytes available for reading.
  std::size_t available(const base_implementation_type& impl,
      boost::system::error_code& ec) const
  {
    return socket_ops::available(impl.socket_, ec);
  }

  // Place the socket into the state where it will listen for new connections.
  boost::system::error_code listen(base_implementation_type& impl,
      int backlog, boost::system::error_code& ec)
  {
    socket_ops::listen(impl.socket_, backlog, ec);
    return ec;
  }

  // Perform an IO control command on the socket.
  template <typename IO_Control_Command>
  boost::system::error_code io_control(base_implementation_type& impl,
      IO_Control_Command& command, boost::system::error_code& ec)
  {
    socket_ops::ioctl(impl.socket_, impl.state_, command.name(),
        static_cast<ioctl_arg_type*>(command.data()), ec);
    return ec;
  }

  // Gets the non-blocking mode of the socket.
  bool non_blocking(const base_implementation_type& impl) const
  {
    return (impl.state_ & socket_ops::user_set_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the socket.
  boost::system::error_code non_blocking(base_implementation_type& impl,
      bool mode, boost::system::error_code& ec)
  {
    socket_ops::set_user_non_blocking(impl.socket_, impl.state_, mode, ec);
    return ec;
  }

  // Gets the non-blocking mode of the native socket implementation.
  bool native_non_blocking(const base_implementation_type& impl) const
  {
    return (impl.state_ & socket_ops::internal_non_blocking) != 0;
  }

  // Sets the non-blocking mode of the native socket implementation.
  boost::system::error_code native_non_blocking(base_implementation_type& impl,
      bool mode, boost::system::error_code& ec)
  {
    socket_ops::set_internal_non_blocking(impl.socket_, impl.state_, mode, ec);
    return ec;
  }

  // Wait for the socket to become ready to read, ready to write, or to have
  // pending error conditions.
  boost::system::error_code wait(base_implementation_type& impl,
      socket_base::wait_type w, boost::system::error_code& ec)
  {
    switch (w)
    {
    case socket_base::wait_read:
      socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);
      break;
    case socket_base::wait_write:
      socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);
      break;
    case socket_base::wait_error:
      socket_ops::poll_error(impl.socket_, impl.state_, -1, ec);
      break;
    default:
      ec = boost::asio::error::invalid_argument;
      break;
    }

    return ec;
  }

  // Asynchronously wait for the socket to become ready to read, ready to
  // write, or to have pending error conditions.
  template <typename Handler, typename IoExecutor>
  void async_wait(base_implementation_type& impl,
      socket_base::wait_type w, Handler& handler, const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    bool is_continuation =
      boost_asio_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_wait_op<Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_wait"));

    // Optionally register for per-operation cancellation.
    operation* iocp_op = p.p;
    if (slot.is_connected())
    {
      p.p->cancellation_key_ = iocp_op =
        &slot.template emplace<reactor_op_cancellation>(
            impl.socket_, iocp_op);
    }

    int op_type = -1;
    switch (w)
    {
      case socket_base::wait_read:
        op_type = start_null_buffers_receive_op(impl, 0, p.p, iocp_op);
        break;
      case socket_base::wait_write:
        op_type = select_reactor::write_op;
        start_reactor_op(impl, select_reactor::write_op, p.p);
        break;
      case socket_base::wait_error:
        op_type = select_reactor::read_op;
        start_reactor_op(impl, select_reactor::except_op, p.p);
        break;
      default:
        p.p->ec_ = boost::asio::error::invalid_argument;
        iocp_service_.post_immediate_completion(p.p, is_continuation);
        break;
    }

    p.v = p.p = 0;

    // Update cancellation method if the reactor was used.
    if (slot.is_connected() && op_type != -1)
    {
      static_cast<reactor_op_cancellation*>(iocp_op)->use_reactor(
          &get_reactor(), &impl.reactor_data_, op_type);
    }
  }

  // Send the given data to the peer. Returns the number of bytes sent.
  template <typename ConstBufferSequence>
  size_t send(base_implementation_type& impl,
      const ConstBufferSequence& buffers,
      socket_base::message_flags flags, boost::system::error_code& ec)
  {
    buffer_sequence_adapter<boost::asio::const_buffer,
        ConstBufferSequence> bufs(buffers);

    return socket_ops::sync_send(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
  }

  // Wait until data can be sent without blocking.
  size_t send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, boost::system::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);

    return 0;
  }

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler, typename IoExecutor>
  void async_send(base_implementation_type& impl,
      const ConstBufferSequence& buffers, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_send_op<
        ConstBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    operation* o = p.p = new (p.v) op(
        impl.cancel_token_, buffers, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_send"));

    buffer_sequence_adapter<boost::asio::const_buffer,
        ConstBufferSequence> bufs(buffers);

    // Optionally register for per-operation cancellation.
    if (slot.is_connected())
      o = &slot.template emplace<iocp_op_cancellation>(impl.socket_, o);

    start_send_op(impl, bufs.buffers(), bufs.count(), flags,
        (impl.state_ & socket_ops::stream_oriented) != 0 && bufs.all_empty(),
        o);
    p.v = p.p = 0;
  }

  // Start an asynchronous wait until data can be sent without blocking.
  template <typename Handler, typename IoExecutor>
  void async_send(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, Handler& handler, const IoExecutor& io_ex)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_null_buffers_op<Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_send(null_buffers)"));

    start_reactor_op(impl, select_reactor::write_op, p.p);
    p.v = p.p = 0;
  }

  // Receive some data from the peer. Returns the number of bytes received.
  template <typename MutableBufferSequence>
  size_t receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags flags, boost::system::error_code& ec)
  {
    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    return socket_ops::sync_recv(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), flags, bufs.all_empty(), ec);
  }

  // Wait until data can be received without blocking.
  size_t receive(base_implementation_type& impl, const null_buffers&,
      socket_base::message_flags, boost::system::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_receive(base_implementation_type& impl,
      const MutableBufferSequence& buffers, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_recv_op<
        MutableBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    operation* o = p.p = new (p.v) op(impl.state_,
        impl.cancel_token_, buffers, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive"));

    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    // Optionally register for per-operation cancellation.
    if (slot.is_connected())
      o = &slot.template emplace<iocp_op_cancellation>(impl.socket_, o);

    start_receive_op(impl, bufs.buffers(), bufs.count(), flags,
        (impl.state_ & socket_ops::stream_oriented) != 0 && bufs.all_empty(),
        o);
    p.v = p.p = 0;
  }

  // Wait until data can be received without blocking.
  template <typename Handler, typename IoExecutor>
  void async_receive(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags flags,
      Handler& handler, const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_null_buffers_op<Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive(null_buffers)"));

    // Optionally register for per-operation cancellation.
    operation* iocp_op = p.p;
    if (slot.is_connected())
    {
      p.p->cancellation_key_ = iocp_op =
        &slot.template emplace<reactor_op_cancellation>(
            impl.socket_, iocp_op);
    }

    int op_type = start_null_buffers_receive_op(impl, flags, p.p, iocp_op);
    p.v = p.p = 0;

    // Update cancellation method if the reactor was used.
    if (slot.is_connected() && op_type != -1)
    {
      static_cast<reactor_op_cancellation*>(iocp_op)->use_reactor(
          &get_reactor(), &impl.reactor_data_, op_type);
    }
  }

  // Receive some data with associated flags. Returns the number of bytes
  // received.
  template <typename MutableBufferSequence>
  size_t receive_with_flags(base_implementation_type& impl,
      const MutableBufferSequence& buffers,
      socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, boost::system::error_code& ec)
  {
    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    return socket_ops::sync_recvmsg(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), in_flags, out_flags, ec);
  }

  // Wait until data can be received without blocking.
  size_t receive_with_flags(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags,
      socket_base::message_flags& out_flags, boost::system::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    // Clear out_flags, since we cannot give it any other sensible value when
    // performing a null_buffers operation.
    out_flags = 0;

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received
  // must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence,
      typename Handler, typename IoExecutor>
  void async_receive_with_flags(base_implementation_type& impl,
      const MutableBufferSequence& buffers, socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, Handler& handler,
      const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_recvmsg_op<
        MutableBufferSequence, Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    operation* o = p.p = new (p.v) op(impl.cancel_token_,
        buffers, out_flags, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive_with_flags"));

    buffer_sequence_adapter<boost::asio::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    // Optionally register for per-operation cancellation.
    if (slot.is_connected())
      o = &slot.template emplace<iocp_op_cancellation>(impl.socket_, o);

    start_receive_op(impl, bufs.buffers(), bufs.count(), in_flags, false, o);
    p.v = p.p = 0;
  }

  // Wait until data can be received without blocking.
  template <typename Handler, typename IoExecutor>
  void async_receive_with_flags(base_implementation_type& impl,
      const null_buffers&, socket_base::message_flags in_flags,
      socket_base::message_flags& out_flags, Handler& handler,
      const IoExecutor& io_ex)
  {
    typename associated_cancellation_slot<Handler>::type slot
      = boost::asio::get_associated_cancellation_slot(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_null_buffers_op<Handler, IoExecutor> op;
    typename op::ptr p = { boost::asio::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler, io_ex);

    BOOST_ASIO_HANDLER_CREATION((context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive_with_flags(null_buffers)"));

    // Reset out_flags since it can be given no sensible value at this time.
    out_flags = 0;

    // Optionally register for per-operation cancellation.
    operation* iocp_op = p.p;
    if (slot.is_connected())
    {
      p.p->cancellation_key_ = iocp_op =
        &slot.template emplace<reactor_op_cancellation>(
            impl.socket_, iocp_op);
    }

    int op_type = start_null_buffers_receive_op(impl, in_flags, p.p, iocp_op);
    p.v = p.p = 0;

    // Update cancellation method if the reactor was used.
    if (slot.is_connected() && op_type != -1)
    {
      static_cast<reactor_op_cancellation*>(iocp_op)->use_reactor(
          &get_reactor(), &impl.reactor_data_, op_type);
    }
  }

  // Helper function to restart an asynchronous accept operation.
  BOOST_ASIO_DECL void restart_accept_op(socket_type s,
      socket_holder& new_socket, int family, int type,
      int protocol, void* output_buffer, DWORD address_length,
      long* cancel_requested, operation* op);

protected:
  // Open a new socket implementation.
  BOOST_ASIO_DECL boost::system::error_code do_open(
      base_implementation_type& impl, int family, int type,
      int protocol, boost::system::error_code& ec);

  // Assign a native socket to a socket implementation.
  BOOST_ASIO_DECL boost::system::error_code do_assign(
      base_implementation_type& impl, int type,
      socket_type native_socket, boost::system::error_code& ec);

  // Helper function to start an asynchronous send operation.
  BOOST_ASIO_DECL void start_send_op(base_implementation_type& impl,
      WSABUF* buffers, std::size_t buffer_count,
      socket_base::message_flags flags, bool noop, operation* op);

  // Helper function to start an asynchronous send_to operation.
  BOOST_ASIO_DECL void start_send_to_op(base_implementation_type& impl,
      WSABUF* buffers, std::size_t buffer_count,
      const socket_addr_type* addr, int addrlen,
      socket_base::message_flags flags, operation* op);

  // Helper function to start an asynchronous receive operation.
  BOOST_ASIO_DECL void start_receive_op(base_implementation_type& impl,
      WSABUF* buffers, std::size_t buffer_count,
      socket_base::message_flags flags, bool noop, operation* op);

  // Helper function to start an asynchronous null_buffers receive operation.
  BOOST_ASIO_DECL int start_null_buffers_receive_op(
      base_implementation_type& impl, socket_base::message_flags flags,
      reactor_op* op, operation* iocp_op);

  // Helper function to start an asynchronous receive_from operation.
  BOOST_ASIO_DECL void start_receive_from_op(base_implementation_type& impl,
      WSABUF* buffers, std::size_t buffer_count, socket_addr_type* addr,
      socket_base::message_flags flags, int* addrlen, operation* op);

  // Helper function to start an asynchronous accept operation.
  BOOST_ASIO_DECL void start_accept_op(base_implementation_type& impl,
      bool peer_is_open, socket_holder& new_socket, int family, int type,
      int protocol, void* output_buffer, DWORD address_length, operation* op);

  // Start an asynchronous read or write operation using the reactor.
  BOOST_ASIO_DECL void start_reactor_op(base_implementation_type& impl,
      int op_type, reactor_op* op);

  // Start the asynchronous connect operation using the reactor.
  BOOST_ASIO_DECL int start_connect_op(base_implementation_type& impl,
      int family, int type, const socket_addr_type* remote_addr,
      std::size_t remote_addrlen, win_iocp_socket_connect_op_base* op,
      operation* iocp_op);

  // Helper function to close a socket when the associated object is being
  // destroyed.
  BOOST_ASIO_DECL void close_for_destruction(base_implementation_type& impl);

  // Update the ID of the thread from which cancellation is safe.
  BOOST_ASIO_DECL void update_cancellation_thread_id(
      base_implementation_type& impl);

  // Helper function to get the reactor. If no reactor has been created yet, a
  // new one is obtained from the execution context and a pointer to it is
  // cached in this service.
  BOOST_ASIO_DECL select_reactor& get_reactor();

  // The type of a ConnectEx function pointer, as old SDKs may not provide it.
  typedef BOOL (PASCAL *connect_ex_fn)(SOCKET,
      const socket_addr_type*, int, void*, DWORD, DWORD*, OVERLAPPED*);

  // Helper function to get the ConnectEx pointer. If no ConnectEx pointer has
  // been obtained yet, one is obtained using WSAIoctl and the pointer is
  // cached. Returns a null pointer if ConnectEx is not available.
  BOOST_ASIO_DECL connect_ex_fn get_connect_ex(
      base_implementation_type& impl, int type);

  // The type of a NtSetInformationFile function pointer.
  typedef LONG (NTAPI *nt_set_info_fn)(HANDLE, ULONG_PTR*, void*, ULONG, ULONG);

  // Helper function to get the NtSetInformationFile function pointer. If no
  // NtSetInformationFile pointer has been obtained yet, one is obtained using
  // GetProcAddress and the pointer is cached. Returns a null pointer if
  // NtSetInformationFile is not available.
  BOOST_ASIO_DECL nt_set_info_fn get_nt_set_info();

  // Helper function to emulate InterlockedCompareExchangePointer functionality
  // for:
  // - very old Platform SDKs; and
  // - platform SDKs where MSVC's /Wp64 option causes spurious warnings.
  BOOST_ASIO_DECL void* interlocked_compare_exchange_pointer(
      void** dest, void* exch, void* cmp);

  // Helper function to emulate InterlockedExchangePointer functionality for:
  // - very old Platform SDKs; and
  // - platform SDKs where MSVC's /Wp64 option causes spurious warnings.
  BOOST_ASIO_DECL void* interlocked_exchange_pointer(void** dest, void* val);

  // Helper class used to implement per operation cancellation.
  class iocp_op_cancellation : public operation
  {
  public:
    iocp_op_cancellation(SOCKET s, operation* target)
      : operation(&iocp_op_cancellation::do_complete),
        socket_(s),
        target_(target)
    {
    }

    static void do_complete(void* owner, operation* base,
        const boost::system::error_code& result_ec,
        std::size_t bytes_transferred)
    {
      iocp_op_cancellation* o = static_cast<iocp_op_cancellation*>(base);
      o->target_->complete(owner, result_ec, bytes_transferred);
    }

    void operator()(cancellation_type_t type)
    {
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        HANDLE sock_as_handle = reinterpret_cast<HANDLE>(socket_);
        ::CancelIoEx(sock_as_handle, this);
      }
#else // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
      (void)type;
#endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
    }

  private:
    SOCKET socket_;
    operation* target_;
  };

  // Helper class used to implement per operation cancellation.
  class accept_op_cancellation : public operation
  {
  public:
    accept_op_cancellation(SOCKET s, operation* target)
      : operation(&iocp_op_cancellation::do_complete),
        socket_(s),
        target_(target),
        cancel_requested_(0)
    {
    }

    static void do_complete(void* owner, operation* base,
        const boost::system::error_code& result_ec,
        std::size_t bytes_transferred)
    {
      accept_op_cancellation* o = static_cast<accept_op_cancellation*>(base);
      o->target_->complete(owner, result_ec, bytes_transferred);
    }

    long* get_cancel_requested()
    {
      return &cancel_requested_;
    }

    void operator()(cancellation_type_t type)
    {
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        HANDLE sock_as_handle = reinterpret_cast<HANDLE>(socket_);
        ::CancelIoEx(sock_as_handle, this);
      }
#else // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
      (void)type;
#endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
    }

  private:
    SOCKET socket_;
    operation* target_;
    long cancel_requested_;
  };

  // Helper class used to implement per operation cancellation.
  class reactor_op_cancellation : public operation
  {
  public:
    reactor_op_cancellation(SOCKET s, operation* base)
      : operation(&reactor_op_cancellation::do_complete),
        socket_(s),
        target_(base),
        reactor_(0),
        reactor_data_(0),
        op_type_(-1)
    {
    }

    void use_reactor(select_reactor* r,
        select_reactor::per_descriptor_data* p, int o)
    {
      reactor_ = r;
      reactor_data_ = p;
      op_type_ = o;
    }

    static void do_complete(void* owner, operation* base,
        const boost::system::error_code& result_ec,
        std::size_t bytes_transferred)
    {
      reactor_op_cancellation* o = static_cast<reactor_op_cancellation*>(base);
      o->target_->complete(owner, result_ec, bytes_transferred);
    }

    void operator()(cancellation_type_t type)
    {
      if (!!(type &
            (cancellation_type::terminal
              | cancellation_type::partial
              | cancellation_type::total)))
      {
        if (reactor_)
        {
          reactor_->cancel_ops_by_key(socket_,
              *reactor_data_, op_type_, this);
        }
        else
        {
#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
          HANDLE sock_as_handle = reinterpret_cast<HANDLE>(socket_);
          ::CancelIoEx(sock_as_handle, this);
#endif // defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
        }
      }
    }

  private:
    SOCKET socket_;
    operation* target_;
    select_reactor* reactor_;
    select_reactor::per_descriptor_data* reactor_data_;
    int op_type_;
  };

  // The execution context used to obtain the reactor, if required.
  execution_context& context_;

  // The IOCP service used for running asynchronous operations and dispatching
  // handlers.
  win_iocp_io_context& iocp_service_;

  // The reactor used for performing connect operations. This object is created
  // only if needed.
  select_reactor* reactor_;

  // Pointer to ConnectEx implementation.
  void* connect_ex_;

  // Pointer to NtSetInformationFile implementation.
  void* nt_set_info_;

  // Mutex to protect access to the linked list of implementations. 
  boost::asio::detail::mutex mutex_;

  // The head of a linked list of all implementations.
  base_implementation_type* impl_list_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/win_iocp_socket_service_base.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // defined(BOOST_ASIO_HAS_IOCP)

#endif // BOOST_ASIO_DETAIL_WIN_IOCP_SOCKET_SERVICE_BASE_HPP
