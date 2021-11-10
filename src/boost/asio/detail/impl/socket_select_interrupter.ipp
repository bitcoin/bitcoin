//
// detail/impl/socket_select_interrupter.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_SOCKET_SELECT_INTERRUPTER_IPP
#define BOOST_ASIO_DETAIL_IMPL_SOCKET_SELECT_INTERRUPTER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#if defined(BOOST_ASIO_WINDOWS) \
  || defined(__CYGWIN__) \
  || defined(__SYMBIAN32__)

#include <cstdlib>
#include <boost/asio/detail/socket_holder.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/socket_select_interrupter.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

socket_select_interrupter::socket_select_interrupter()
{
  open_descriptors();
}

void socket_select_interrupter::open_descriptors()
{
  boost::system::error_code ec;
  socket_holder acceptor(socket_ops::socket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP, ec));
  if (acceptor.get() == invalid_socket)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  int opt = 1;
  socket_ops::state_type acceptor_state = 0;
  socket_ops::setsockopt(acceptor.get(), acceptor_state,
      SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt), ec);

  using namespace std; // For memset.
  sockaddr_in4_type addr;
  std::size_t addr_len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = socket_ops::host_to_network_long(INADDR_LOOPBACK);
  addr.sin_port = 0;
  if (socket_ops::bind(acceptor.get(), (const socket_addr_type*)&addr,
        addr_len, ec) == socket_error_retval)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  if (socket_ops::getsockname(acceptor.get(), (socket_addr_type*)&addr,
        &addr_len, ec) == socket_error_retval)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  // Some broken firewalls on Windows will intermittently cause getsockname to
  // return 0.0.0.0 when the socket is actually bound to 127.0.0.1. We
  // explicitly specify the target address here to work around this problem.
  if (addr.sin_addr.s_addr == socket_ops::host_to_network_long(INADDR_ANY))
    addr.sin_addr.s_addr = socket_ops::host_to_network_long(INADDR_LOOPBACK);

  if (socket_ops::listen(acceptor.get(),
        SOMAXCONN, ec) == socket_error_retval)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  socket_holder client(socket_ops::socket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP, ec));
  if (client.get() == invalid_socket)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  if (socket_ops::connect(client.get(), (const socket_addr_type*)&addr,
        addr_len, ec) == socket_error_retval)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  socket_holder server(socket_ops::accept(acceptor.get(), 0, 0, ec));
  if (server.get() == invalid_socket)
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");
  
  ioctl_arg_type non_blocking = 1;
  socket_ops::state_type client_state = 0;
  if (socket_ops::ioctl(client.get(), client_state,
        FIONBIO, &non_blocking, ec))
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  opt = 1;
  socket_ops::setsockopt(client.get(), client_state,
      IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt), ec);

  non_blocking = 1;
  socket_ops::state_type server_state = 0;
  if (socket_ops::ioctl(server.get(), server_state,
        FIONBIO, &non_blocking, ec))
    boost::asio::detail::throw_error(ec, "socket_select_interrupter");

  opt = 1;
  socket_ops::setsockopt(server.get(), server_state,
      IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt), ec);

  read_descriptor_ = server.release();
  write_descriptor_ = client.release();
}

socket_select_interrupter::~socket_select_interrupter()
{
  close_descriptors();
}

void socket_select_interrupter::close_descriptors()
{
  boost::system::error_code ec;
  socket_ops::state_type state = socket_ops::internal_non_blocking;
  if (read_descriptor_ != invalid_socket)
    socket_ops::close(read_descriptor_, state, true, ec);
  if (write_descriptor_ != invalid_socket)
    socket_ops::close(write_descriptor_, state, true, ec);
}

void socket_select_interrupter::recreate()
{
  close_descriptors();

  write_descriptor_ = invalid_socket;
  read_descriptor_ = invalid_socket;

  open_descriptors();
}

void socket_select_interrupter::interrupt()
{
  char byte = 0;
  socket_ops::buf b;
  socket_ops::init_buf(b, &byte, 1);
  boost::system::error_code ec;
  socket_ops::send(write_descriptor_, &b, 1, 0, ec);
}

bool socket_select_interrupter::reset()
{
  char data[1024];
  socket_ops::buf b;
  socket_ops::init_buf(b, data, sizeof(data));
  boost::system::error_code ec;
  for (;;)
  {
    int bytes_read = socket_ops::recv(read_descriptor_, &b, 1, 0, ec);
    if (bytes_read == sizeof(data))
      continue;
    if (bytes_read > 0)
      return true;
    if (bytes_read == 0)
      return false;
    if (ec == boost::asio::error::would_block
        || ec == boost::asio::error::try_again)
      return true;
    return false;
  }
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_WINDOWS)
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)

#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#endif // BOOST_ASIO_DETAIL_IMPL_SOCKET_SELECT_INTERRUPTER_IPP
