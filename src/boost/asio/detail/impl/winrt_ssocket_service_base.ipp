//
// detail/impl/winrt_ssocket_service_base.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_WINRT_SSOCKET_SERVICE_BASE_IPP
#define BOOST_ASIO_DETAIL_IMPL_WINRT_SSOCKET_SERVICE_BASE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)

#include <cstring>
#include <boost/asio/detail/winrt_ssocket_service_base.hpp>
#include <boost/asio/detail/winrt_async_op.hpp>
#include <boost/asio/detail/winrt_utils.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

winrt_ssocket_service_base::winrt_ssocket_service_base(
    execution_context& context)
  : scheduler_(use_service<scheduler_impl>(context)),
    async_manager_(use_service<winrt_async_manager>(context)),
    mutex_(),
    impl_list_(0)
{
}

void winrt_ssocket_service_base::base_shutdown()
{
  // Close all implementations, causing all operations to complete.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  base_implementation_type* impl = impl_list_;
  while (impl)
  {
    boost::system::error_code ignored_ec;
    close(*impl, ignored_ec);
    impl = impl->next_;
  }
}

void winrt_ssocket_service_base::construct(
    winrt_ssocket_service_base::base_implementation_type& impl)
{
  // Insert implementation into linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void winrt_ssocket_service_base::base_move_construct(
    winrt_ssocket_service_base::base_implementation_type& impl,
    winrt_ssocket_service_base::base_implementation_type& other_impl)
  BOOST_ASIO_NOEXCEPT
{
  impl.socket_ = other_impl.socket_;
  other_impl.socket_ = nullptr;

  // Insert implementation into linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void winrt_ssocket_service_base::base_move_assign(
    winrt_ssocket_service_base::base_implementation_type& impl,
    winrt_ssocket_service_base& other_service,
    winrt_ssocket_service_base::base_implementation_type& other_impl)
{
  boost::system::error_code ignored_ec;
  close(impl, ignored_ec);

  if (this != &other_service)
  {
    // Remove implementation from linked list of all implementations.
    boost::asio::detail::mutex::scoped_lock lock(mutex_);
    if (impl_list_ == &impl)
      impl_list_ = impl.next_;
    if (impl.prev_)
      impl.prev_->next_ = impl.next_;
    if (impl.next_)
      impl.next_->prev_= impl.prev_;
    impl.next_ = 0;
    impl.prev_ = 0;
  }

  impl.socket_ = other_impl.socket_;
  other_impl.socket_ = nullptr;

  if (this != &other_service)
  {
    // Insert implementation into linked list of all implementations.
    boost::asio::detail::mutex::scoped_lock lock(other_service.mutex_);
    impl.next_ = other_service.impl_list_;
    impl.prev_ = 0;
    if (other_service.impl_list_)
      other_service.impl_list_->prev_ = &impl;
    other_service.impl_list_ = &impl;
  }
}

void winrt_ssocket_service_base::destroy(
    winrt_ssocket_service_base::base_implementation_type& impl)
{
  boost::system::error_code ignored_ec;
  close(impl, ignored_ec);

  // Remove implementation from linked list of all implementations.
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  if (impl_list_ == &impl)
    impl_list_ = impl.next_;
  if (impl.prev_)
    impl.prev_->next_ = impl.next_;
  if (impl.next_)
    impl.next_->prev_= impl.prev_;
  impl.next_ = 0;
  impl.prev_ = 0;
}

boost::system::error_code winrt_ssocket_service_base::close(
    winrt_ssocket_service_base::base_implementation_type& impl,
    boost::system::error_code& ec)
{
  delete impl.socket_;
  impl.socket_ = nullptr;
  ec = boost::system::error_code();
  return ec;
}

winrt_ssocket_service_base::native_handle_type
winrt_ssocket_service_base::release(
    winrt_ssocket_service_base::base_implementation_type& impl,
    boost::system::error_code& ec)
{
  if (!is_open(impl))
    return nullptr;

  cancel(impl, ec);
  if (ec)
    return nullptr;

  native_handle_type tmp = impl.socket_;
  impl.socket_ = nullptr;
  return tmp;
}

std::size_t winrt_ssocket_service_base::do_get_endpoint(
    const base_implementation_type& impl, bool local,
    void* addr, std::size_t addr_len, boost::system::error_code& ec) const
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return addr_len;
  }

  try
  {
    std::string addr_string = winrt_utils::string(local
        ? impl.socket_->Information->LocalAddress->CanonicalName
        : impl.socket_->Information->RemoteAddress->CanonicalName);
    unsigned short port = winrt_utils::integer(local
        ? impl.socket_->Information->LocalPort
        : impl.socket_->Information->RemotePort);
    unsigned long scope = 0;

    switch (reinterpret_cast<const socket_addr_type*>(addr)->sa_family)
    {
    case BOOST_ASIO_OS_DEF(AF_INET):
      if (addr_len < sizeof(sockaddr_in4_type))
      {
        ec = boost::asio::error::invalid_argument;
        return addr_len;
      }
      else
      {
        socket_ops::inet_pton(BOOST_ASIO_OS_DEF(AF_INET), addr_string.c_str(),
            &reinterpret_cast<sockaddr_in4_type*>(addr)->sin_addr, &scope, ec);
        reinterpret_cast<sockaddr_in4_type*>(addr)->sin_port
          = socket_ops::host_to_network_short(port);
        ec = boost::system::error_code();
        return sizeof(sockaddr_in4_type);
      }
    case BOOST_ASIO_OS_DEF(AF_INET6):
      if (addr_len < sizeof(sockaddr_in6_type))
      {
        ec = boost::asio::error::invalid_argument;
        return addr_len;
      }
      else
      {
        socket_ops::inet_pton(BOOST_ASIO_OS_DEF(AF_INET6), addr_string.c_str(),
            &reinterpret_cast<sockaddr_in6_type*>(addr)->sin6_addr, &scope, ec);
        reinterpret_cast<sockaddr_in6_type*>(addr)->sin6_port
          = socket_ops::host_to_network_short(port);
        ec = boost::system::error_code();
        return sizeof(sockaddr_in6_type);
      }
    default:
      ec = boost::asio::error::address_family_not_supported;
      return addr_len;
    }
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
    return addr_len;
  }
}

boost::system::error_code winrt_ssocket_service_base::do_set_option(
    winrt_ssocket_service_base::base_implementation_type& impl,
    int level, int optname, const void* optval,
    std::size_t optlen, boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return ec;
  }

  try
  {
    if (level == BOOST_ASIO_OS_DEF(SOL_SOCKET)
        && optname == BOOST_ASIO_OS_DEF(SO_KEEPALIVE))
    {
      if (optlen == sizeof(int))
      {
        int value = 0;
        std::memcpy(&value, optval, optlen);
        impl.socket_->Control->KeepAlive = !!value;
        ec = boost::system::error_code();
      }
      else
      {
        ec = boost::asio::error::invalid_argument;
      }
    }
    else if (level == BOOST_ASIO_OS_DEF(IPPROTO_TCP)
        && optname == BOOST_ASIO_OS_DEF(TCP_NODELAY))
    {
      if (optlen == sizeof(int))
      {
        int value = 0;
        std::memcpy(&value, optval, optlen);
        impl.socket_->Control->NoDelay = !!value;
        ec = boost::system::error_code();
      }
      else
      {
        ec = boost::asio::error::invalid_argument;
      }
    }
    else
    {
      ec = boost::asio::error::invalid_argument;
    }
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
  }

  return ec;
}

void winrt_ssocket_service_base::do_get_option(
    const winrt_ssocket_service_base::base_implementation_type& impl,
    int level, int optname, void* optval,
    std::size_t* optlen, boost::system::error_code& ec) const
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return;
  }

  try
  {
    if (level == BOOST_ASIO_OS_DEF(SOL_SOCKET)
        && optname == BOOST_ASIO_OS_DEF(SO_KEEPALIVE))
    {
      if (*optlen >= sizeof(int))
      {
        int value = impl.socket_->Control->KeepAlive ? 1 : 0;
        std::memcpy(optval, &value, sizeof(int));
        *optlen = sizeof(int);
        ec = boost::system::error_code();
      }
      else
      {
        ec = boost::asio::error::invalid_argument;
      }
    }
    else if (level == BOOST_ASIO_OS_DEF(IPPROTO_TCP)
        && optname == BOOST_ASIO_OS_DEF(TCP_NODELAY))
    {
      if (*optlen >= sizeof(int))
      {
        int value = impl.socket_->Control->NoDelay ? 1 : 0;
        std::memcpy(optval, &value, sizeof(int));
        *optlen = sizeof(int);
        ec = boost::system::error_code();
      }
      else
      {
        ec = boost::asio::error::invalid_argument;
      }
    }
    else
    {
      ec = boost::asio::error::invalid_argument;
    }
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
  }
}

boost::system::error_code winrt_ssocket_service_base::do_connect(
    winrt_ssocket_service_base::base_implementation_type& impl,
    const void* addr, boost::system::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return ec;
  }

  char addr_string[max_addr_v6_str_len];
  unsigned short port;
  switch (reinterpret_cast<const socket_addr_type*>(addr)->sa_family)
  {
  case BOOST_ASIO_OS_DEF(AF_INET):
    socket_ops::inet_ntop(BOOST_ASIO_OS_DEF(AF_INET),
        &reinterpret_cast<const sockaddr_in4_type*>(addr)->sin_addr,
        addr_string, sizeof(addr_string), 0, ec);
    port = socket_ops::network_to_host_short(
        reinterpret_cast<const sockaddr_in4_type*>(addr)->sin_port);
    break;
  case BOOST_ASIO_OS_DEF(AF_INET6):
    socket_ops::inet_ntop(BOOST_ASIO_OS_DEF(AF_INET6),
        &reinterpret_cast<const sockaddr_in6_type*>(addr)->sin6_addr,
        addr_string, sizeof(addr_string), 0, ec);
    port = socket_ops::network_to_host_short(
        reinterpret_cast<const sockaddr_in6_type*>(addr)->sin6_port);
    break;
  default:
    ec = boost::asio::error::address_family_not_supported;
    return ec;
  }

  if (!ec) try
  {
    async_manager_.sync(impl.socket_->ConnectAsync(
          ref new Windows::Networking::HostName(
            winrt_utils::string(addr_string)),
          winrt_utils::string(port)), ec);
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
  }

  return ec;
}

void winrt_ssocket_service_base::start_connect_op(
    winrt_ssocket_service_base::base_implementation_type& impl,
    const void* addr, winrt_async_op<void>* op, bool is_continuation)
{
  if (!is_open(impl))
  {
    op->ec_ = boost::asio::error::bad_descriptor;
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  char addr_string[max_addr_v6_str_len];
  unsigned short port = 0;
  switch (reinterpret_cast<const socket_addr_type*>(addr)->sa_family)
  {
  case BOOST_ASIO_OS_DEF(AF_INET):
    socket_ops::inet_ntop(BOOST_ASIO_OS_DEF(AF_INET),
        &reinterpret_cast<const sockaddr_in4_type*>(addr)->sin_addr,
        addr_string, sizeof(addr_string), 0, op->ec_);
    port = socket_ops::network_to_host_short(
        reinterpret_cast<const sockaddr_in4_type*>(addr)->sin_port);
    break;
  case BOOST_ASIO_OS_DEF(AF_INET6):
    socket_ops::inet_ntop(BOOST_ASIO_OS_DEF(AF_INET6),
        &reinterpret_cast<const sockaddr_in6_type*>(addr)->sin6_addr,
        addr_string, sizeof(addr_string), 0, op->ec_);
    port = socket_ops::network_to_host_short(
        reinterpret_cast<const sockaddr_in6_type*>(addr)->sin6_port);
    break;
  default:
    op->ec_ = boost::asio::error::address_family_not_supported;
    break;
  }

  if (op->ec_)
  {
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  try
  {
    async_manager_.async(impl.socket_->ConnectAsync(
          ref new Windows::Networking::HostName(
            winrt_utils::string(addr_string)),
          winrt_utils::string(port)), op);
  }
  catch (Platform::Exception^ e)
  {
    op->ec_ = boost::system::error_code(
        e->HResult, boost::system::system_category());
    scheduler_.post_immediate_completion(op, is_continuation);
  }
}

std::size_t winrt_ssocket_service_base::do_send(
    winrt_ssocket_service_base::base_implementation_type& impl,
    const boost::asio::const_buffer& data,
    socket_base::message_flags flags, boost::system::error_code& ec)
{
  if (flags)
  {
    ec = boost::asio::error::operation_not_supported;
    return 0;
  }

  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  try
  {
    buffer_sequence_adapter<boost::asio::const_buffer,
      boost::asio::const_buffer> bufs(boost::asio::buffer(data));

    if (bufs.all_empty())
    {
      ec = boost::system::error_code();
      return 0;
    }

    return async_manager_.sync(
        impl.socket_->OutputStream->WriteAsync(bufs.buffers()[0]), ec);
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
    return 0;
  }
}

void winrt_ssocket_service_base::start_send_op(
      winrt_ssocket_service_base::base_implementation_type& impl,
      const boost::asio::const_buffer& data, socket_base::message_flags flags,
      winrt_async_op<unsigned int>* op, bool is_continuation)
{
  if (flags)
  {
    op->ec_ = boost::asio::error::operation_not_supported;
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  if (!is_open(impl))
  {
    op->ec_ = boost::asio::error::bad_descriptor;
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  try
  {
    buffer_sequence_adapter<boost::asio::const_buffer,
        boost::asio::const_buffer> bufs(boost::asio::buffer(data));

    if (bufs.all_empty())
    {
      scheduler_.post_immediate_completion(op, is_continuation);
      return;
    }

    async_manager_.async(
        impl.socket_->OutputStream->WriteAsync(bufs.buffers()[0]), op);
  }
  catch (Platform::Exception^ e)
  {
    op->ec_ = boost::system::error_code(e->HResult,
        boost::system::system_category());
    scheduler_.post_immediate_completion(op, is_continuation);
  }
}

std::size_t winrt_ssocket_service_base::do_receive(
    winrt_ssocket_service_base::base_implementation_type& impl,
    const boost::asio::mutable_buffer& data,
    socket_base::message_flags flags, boost::system::error_code& ec)
{
  if (flags)
  {
    ec = boost::asio::error::operation_not_supported;
    return 0;
  }

  if (!is_open(impl))
  {
    ec = boost::asio::error::bad_descriptor;
    return 0;
  }

  try
  {
    buffer_sequence_adapter<boost::asio::mutable_buffer,
        boost::asio::mutable_buffer> bufs(boost::asio::buffer(data));

    if (bufs.all_empty())
    {
      ec = boost::system::error_code();
      return 0;
    }

    async_manager_.sync(
        impl.socket_->InputStream->ReadAsync(
          bufs.buffers()[0], bufs.buffers()[0]->Capacity,
          Windows::Storage::Streams::InputStreamOptions::Partial), ec);

    std::size_t bytes_transferred = bufs.buffers()[0]->Length;
    if (bytes_transferred == 0 && !ec)
    {
      ec = boost::asio::error::eof;
    }

    return bytes_transferred;
  }
  catch (Platform::Exception^ e)
  {
    ec = boost::system::error_code(e->HResult,
        boost::system::system_category());
    return 0;
  }
}

void winrt_ssocket_service_base::start_receive_op(
      winrt_ssocket_service_base::base_implementation_type& impl,
      const boost::asio::mutable_buffer& data, socket_base::message_flags flags,
      winrt_async_op<Windows::Storage::Streams::IBuffer^>* op,
      bool is_continuation)
{
  if (flags)
  {
    op->ec_ = boost::asio::error::operation_not_supported;
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  if (!is_open(impl))
  {
    op->ec_ = boost::asio::error::bad_descriptor;
    scheduler_.post_immediate_completion(op, is_continuation);
    return;
  }

  try
  {
    buffer_sequence_adapter<boost::asio::mutable_buffer,
        boost::asio::mutable_buffer> bufs(boost::asio::buffer(data));

    if (bufs.all_empty())
    {
      scheduler_.post_immediate_completion(op, is_continuation);
      return;
    }

    async_manager_.async(
        impl.socket_->InputStream->ReadAsync(
          bufs.buffers()[0], bufs.buffers()[0]->Capacity,
          Windows::Storage::Streams::InputStreamOptions::Partial), op);
  }
  catch (Platform::Exception^ e)
  {
    op->ec_ = boost::system::error_code(e->HResult,
        boost::system::system_category());
    scheduler_.post_immediate_completion(op, is_continuation);
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)

#endif // BOOST_ASIO_DETAIL_IMPL_WINRT_SSOCKET_SERVICE_BASE_IPP
