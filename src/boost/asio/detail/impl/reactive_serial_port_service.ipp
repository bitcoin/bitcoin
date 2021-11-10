//
// detail/impl/reactive_serial_port_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_REACTIVE_SERIAL_PORT_SERVICE_IPP
#define BOOST_ASIO_DETAIL_IMPL_REACTIVE_SERIAL_PORT_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_SERIAL_PORT)
#if !defined(BOOST_ASIO_WINDOWS) && !defined(__CYGWIN__)

#include <cstring>
#include <boost/asio/detail/reactive_serial_port_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

reactive_serial_port_service::reactive_serial_port_service(
    execution_context& context)
  : execution_context_service_base<reactive_serial_port_service>(context),
    descriptor_service_(context)
{
}

void reactive_serial_port_service::shutdown()
{
  descriptor_service_.shutdown();
}

boost::system::error_code reactive_serial_port_service::open(
    reactive_serial_port_service::implementation_type& impl,
    const std::string& device, boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    ec = boost::asio::error::already_open;
    return ec;
  }

  descriptor_ops::state_type state = 0;
  int fd = descriptor_ops::open(device.c_str(),
      O_RDWR | O_NONBLOCK | O_NOCTTY, ec);
  if (fd < 0)
    return ec;

  int s = descriptor_ops::fcntl(fd, F_GETFL, ec);
  if (s >= 0)
    s = descriptor_ops::fcntl(fd, F_SETFL, s | O_NONBLOCK, ec);
  if (s < 0)
  {
    boost::system::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
    return ec;
  }

  // Set up default serial port options.
  termios ios;
  s = ::tcgetattr(fd, &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s >= 0)
  {
#if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
    ::cfmakeraw(&ios);
#else
    ios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK
        | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    ios.c_oflag &= ~OPOST;
    ios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    ios.c_cflag &= ~(CSIZE | PARENB);
    ios.c_cflag |= CS8;
#endif
    ios.c_iflag |= IGNPAR;
    ios.c_cflag |= CREAD | CLOCAL;
    s = ::tcsetattr(fd, TCSANOW, &ios);
    descriptor_ops::get_last_error(ec, s < 0);
  }
  if (s < 0)
  {
    boost::system::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
    return ec;
  }

  // We're done. Take ownership of the serial port descriptor.
  if (descriptor_service_.assign(impl, fd, ec))
  {
    boost::system::error_code ignored_ec;
    descriptor_ops::close(fd, state, ignored_ec);
  }

  return ec;
}

boost::system::error_code reactive_serial_port_service::do_set_option(
    reactive_serial_port_service::implementation_type& impl,
    reactive_serial_port_service::store_function_type store,
    const void* option, boost::system::error_code& ec)
{
  termios ios;
  int s = ::tcgetattr(descriptor_service_.native_handle(impl), &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s < 0)
    return ec;

  if (store(option, ios, ec))
    return ec;

  s = ::tcsetattr(descriptor_service_.native_handle(impl), TCSANOW, &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  return ec;
}

boost::system::error_code reactive_serial_port_service::do_get_option(
    const reactive_serial_port_service::implementation_type& impl,
    reactive_serial_port_service::load_function_type load,
    void* option, boost::system::error_code& ec) const
{
  termios ios;
  int s = ::tcgetattr(descriptor_service_.native_handle(impl), &ios);
  descriptor_ops::get_last_error(ec, s < 0);
  if (s < 0)
    return ec;

  return load(option, ios, ec);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // !defined(BOOST_ASIO_WINDOWS) && !defined(__CYGWIN__)
#endif // defined(BOOST_ASIO_HAS_SERIAL_PORT)

#endif // BOOST_ASIO_DETAIL_IMPL_REACTIVE_SERIAL_PORT_SERVICE_IPP
