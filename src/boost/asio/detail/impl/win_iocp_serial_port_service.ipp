//
// detail/impl/win_iocp_serial_port_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_SERIAL_PORT_SERVICE_IPP
#define BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_SERIAL_PORT_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_IOCP) && defined(BOOST_ASIO_HAS_SERIAL_PORT)

#include <cstring>
#include <boost/asio/detail/win_iocp_serial_port_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

win_iocp_serial_port_service::win_iocp_serial_port_service(
    execution_context& context)
  : execution_context_service_base<win_iocp_serial_port_service>(context),
    handle_service_(context)
{
}

void win_iocp_serial_port_service::shutdown()
{
}

boost::system::error_code win_iocp_serial_port_service::open(
    win_iocp_serial_port_service::implementation_type& impl,
    const std::string& device, boost::system::error_code& ec)
{
  if (is_open(impl))
  {
    ec = boost::asio::error::already_open;
    return ec;
  }

  // For convenience, add a leading \\.\ sequence if not already present.
  std::string name = (device[0] == '\\') ? device : "\\\\.\\" + device;

  // Open a handle to the serial port.
  ::HANDLE handle = ::CreateFileA(name.c_str(),
      GENERIC_READ | GENERIC_WRITE, 0, 0,
      OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
  if (handle == INVALID_HANDLE_VALUE)
  {
    DWORD last_error = ::GetLastError();
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  // Determine the initial serial port parameters.
  using namespace std; // For memset.
  ::DCB dcb;
  memset(&dcb, 0, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!::GetCommState(handle, &dcb))
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(handle);
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  // Set some default serial port parameters. This implementation does not
  // support changing all of these, so they might as well be in a known state.
  dcb.fBinary = TRUE; // Win32 only supports binary mode.
  dcb.fNull = FALSE; // Do not ignore NULL characters.
  dcb.fAbortOnError = FALSE; // Ignore serial framing errors.
  dcb.BaudRate = CBR_9600; // 9600 baud by default
  dcb.ByteSize = 8; // 8 bit bytes
  dcb.fOutxCtsFlow = FALSE; // No flow control
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fDsrSensitivity = FALSE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;
  dcb.fParity = FALSE; // No parity
  dcb.Parity = NOPARITY;
  dcb.StopBits = ONESTOPBIT; // One stop bit
  if (!::SetCommState(handle, &dcb))
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(handle);
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  // Set up timeouts so that the serial port will behave similarly to a
  // network socket. Reads wait for at least one byte, then return with
  // whatever they have. Writes return once everything is out the door.
  ::COMMTIMEOUTS timeouts;
  timeouts.ReadIntervalTimeout = 1;
  timeouts.ReadTotalTimeoutMultiplier = 0;
  timeouts.ReadTotalTimeoutConstant = 0;
  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;
  if (!::SetCommTimeouts(handle, &timeouts))
  {
    DWORD last_error = ::GetLastError();
    ::CloseHandle(handle);
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  // We're done. Take ownership of the serial port handle.
  if (handle_service_.assign(impl, handle, ec))
    ::CloseHandle(handle);
  return ec;
}

boost::system::error_code win_iocp_serial_port_service::do_set_option(
    win_iocp_serial_port_service::implementation_type& impl,
    win_iocp_serial_port_service::store_function_type store,
    const void* option, boost::system::error_code& ec)
{
  using namespace std; // For memcpy.

  ::DCB dcb;
  memset(&dcb, 0, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!::GetCommState(handle_service_.native_handle(impl), &dcb))
  {
    DWORD last_error = ::GetLastError();
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  if (store(option, dcb, ec))
    return ec;

  if (!::SetCommState(handle_service_.native_handle(impl), &dcb))
  {
    DWORD last_error = ::GetLastError();
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  ec = boost::system::error_code();
  return ec;
}

boost::system::error_code win_iocp_serial_port_service::do_get_option(
    const win_iocp_serial_port_service::implementation_type& impl,
    win_iocp_serial_port_service::load_function_type load,
    void* option, boost::system::error_code& ec) const
{
  using namespace std; // For memset.

  ::DCB dcb;
  memset(&dcb, 0, sizeof(DCB));
  dcb.DCBlength = sizeof(DCB);
  if (!::GetCommState(handle_service_.native_handle(impl), &dcb))
  {
    DWORD last_error = ::GetLastError();
    ec = boost::system::error_code(last_error,
        boost::asio::error::get_system_category());
    return ec;
  }

  return load(option, dcb, ec);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_IOCP) && defined(BOOST_ASIO_HAS_SERIAL_PORT)

#endif // BOOST_ASIO_DETAIL_IMPL_WIN_IOCP_SERIAL_PORT_SERVICE_IPP
