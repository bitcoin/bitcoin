//
// impl/serial_port_base.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_SERIAL_PORT_BASE_IPP
#define BOOST_ASIO_IMPL_SERIAL_PORT_BASE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_SERIAL_PORT)

#include <stdexcept>
#include <boost/asio/error.hpp>
#include <boost/asio/serial_port_base.hpp>
#include <boost/asio/detail/throw_exception.hpp>

#if defined(GENERATING_DOCUMENTATION)
# define BOOST_ASIO_OPTION_STORAGE implementation_defined
#elif defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
# define BOOST_ASIO_OPTION_STORAGE DCB
#else
# define BOOST_ASIO_OPTION_STORAGE termios
#endif

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

BOOST_ASIO_SYNC_OP_VOID serial_port_base::baud_rate::store(
    BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec) const
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  storage.BaudRate = value_;
#else
  speed_t baud;
  switch (value_)
  {
  // Do POSIX-specified rates first.
  case 0: baud = B0; break;
  case 50: baud = B50; break;
  case 75: baud = B75; break;
  case 110: baud = B110; break;
  case 134: baud = B134; break;
  case 150: baud = B150; break;
  case 200: baud = B200; break;
  case 300: baud = B300; break;
  case 600: baud = B600; break;
  case 1200: baud = B1200; break;
  case 1800: baud = B1800; break;
  case 2400: baud = B2400; break;
  case 4800: baud = B4800; break;
  case 9600: baud = B9600; break;
  case 19200: baud = B19200; break;
  case 38400: baud = B38400; break;
  // And now the extended ones conditionally.
# ifdef B7200
  case 7200: baud = B7200; break;
# endif
# ifdef B14400
  case 14400: baud = B14400; break;
# endif
# ifdef B57600
  case 57600: baud = B57600; break;
# endif
# ifdef B115200
  case 115200: baud = B115200; break;
# endif
# ifdef B230400
  case 230400: baud = B230400; break;
# endif
# ifdef B460800
  case 460800: baud = B460800; break;
# endif
# ifdef B500000
  case 500000: baud = B500000; break;
# endif
# ifdef B576000
  case 576000: baud = B576000; break;
# endif
# ifdef B921600
  case 921600: baud = B921600; break;
# endif
# ifdef B1000000
  case 1000000: baud = B1000000; break;
# endif
# ifdef B1152000
  case 1152000: baud = B1152000; break;
# endif
# ifdef B2000000
  case 2000000: baud = B2000000; break;
# endif
# ifdef B3000000
  case 3000000: baud = B3000000; break;
# endif
# ifdef B3500000
  case 3500000: baud = B3500000; break;
# endif
# ifdef B4000000
  case 4000000: baud = B4000000; break;
# endif
  default:
    ec = boost::asio::error::invalid_argument;
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }
# if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
  ::cfsetspeed(&storage, baud);
# else
  ::cfsetispeed(&storage, baud);
  ::cfsetospeed(&storage, baud);
# endif
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::baud_rate::load(
    const BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec)
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  value_ = storage.BaudRate;
#else
  speed_t baud = ::cfgetospeed(&storage);
  switch (baud)
  {
  // First do those specified by POSIX.
  case B0: value_ = 0; break;
  case B50: value_ = 50; break;
  case B75: value_ = 75; break;
  case B110: value_ = 110; break;
  case B134: value_ = 134; break;
  case B150: value_ = 150; break;
  case B200: value_ = 200; break;
  case B300: value_ = 300; break;
  case B600: value_ = 600; break;
  case B1200: value_ = 1200; break;
  case B1800: value_ = 1800; break;
  case B2400: value_ = 2400; break;
  case B4800: value_ = 4800; break;
  case B9600: value_ = 9600; break;
  case B19200: value_ = 19200; break;
  case B38400: value_ = 38400; break;
  // Now conditionally handle a bunch of extended rates.
# ifdef B7200
  case B7200: value_ = 7200; break;
# endif
# ifdef B14400
  case B14400: value_ = 14400; break;
# endif
# ifdef B57600
  case B57600: value_ = 57600; break;
# endif
# ifdef B115200
  case B115200: value_ = 115200; break;
# endif
# ifdef B230400
  case B230400: value_ = 230400; break;
# endif
# ifdef B460800
  case B460800: value_ = 460800; break;
# endif
# ifdef B500000
  case B500000: value_ = 500000; break;
# endif
# ifdef B576000
  case B576000: value_ = 576000; break;
# endif
# ifdef B921600
  case B921600: value_ = 921600; break;
# endif
# ifdef B1000000
  case B1000000: value_ = 1000000; break;
# endif
# ifdef B1152000
  case B1152000: value_ = 1152000; break;
# endif
# ifdef B2000000
  case B2000000: value_ = 2000000; break;
# endif
# ifdef B3000000
  case B3000000: value_ = 3000000; break;
# endif
# ifdef B3500000
  case B3500000: value_ = 3500000; break;
# endif
# ifdef B4000000
  case B4000000: value_ = 4000000; break;
# endif
  default:
    value_ = 0;
    ec = boost::asio::error::invalid_argument;
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

serial_port_base::flow_control::flow_control(
    serial_port_base::flow_control::type t)
  : value_(t)
{
  if (t != none && t != software && t != hardware)
  {
    std::out_of_range ex("invalid flow_control value");
    boost::asio::detail::throw_exception(ex);
  }
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::flow_control::store(
    BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec) const
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  storage.fOutxCtsFlow = FALSE;
  storage.fOutxDsrFlow = FALSE;
  storage.fTXContinueOnXoff = TRUE;
  storage.fDtrControl = DTR_CONTROL_ENABLE;
  storage.fDsrSensitivity = FALSE;
  storage.fOutX = FALSE;
  storage.fInX = FALSE;
  storage.fRtsControl = RTS_CONTROL_ENABLE;
  switch (value_)
  {
  case none:
    break;
  case software:
    storage.fOutX = TRUE;
    storage.fInX = TRUE;
    break;
  case hardware:
    storage.fOutxCtsFlow = TRUE;
    storage.fRtsControl = RTS_CONTROL_HANDSHAKE;
    break;
  default:
    break;
  }
#else
  switch (value_)
  {
  case none:
    storage.c_iflag &= ~(IXOFF | IXON);
# if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
    storage.c_cflag &= ~CRTSCTS;
# elif defined(__QNXNTO__)
    storage.c_cflag &= ~(IHFLOW | OHFLOW);
# endif
    break;
  case software:
    storage.c_iflag |= IXOFF | IXON;
# if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
    storage.c_cflag &= ~CRTSCTS;
# elif defined(__QNXNTO__)
    storage.c_cflag &= ~(IHFLOW | OHFLOW);
# endif
    break;
  case hardware:
# if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
    storage.c_iflag &= ~(IXOFF | IXON);
    storage.c_cflag |= CRTSCTS;
    break;
# elif defined(__QNXNTO__)
    storage.c_iflag &= ~(IXOFF | IXON);
    storage.c_cflag |= (IHFLOW | OHFLOW);
    break;
# else
    ec = boost::asio::error::operation_not_supported;
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
# endif
  default:
    break;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::flow_control::load(
    const BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec)
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  if (storage.fOutX && storage.fInX)
  {
    value_ = software;
  }
  else if (storage.fOutxCtsFlow && storage.fRtsControl == RTS_CONTROL_HANDSHAKE)
  {
    value_ = hardware;
  }
  else
  {
    value_ = none;
  }
#else
  if (storage.c_iflag & (IXOFF | IXON))
  {
    value_ = software;
  }
# if defined(_BSD_SOURCE) || defined(_DEFAULT_SOURCE)
  else if (storage.c_cflag & CRTSCTS)
  {
    value_ = hardware;
  }
# elif defined(__QNXNTO__)
  else if (storage.c_cflag & IHFLOW && storage.c_cflag & OHFLOW)
  {
    value_ = hardware;
  }
# endif
  else
  {
    value_ = none;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

serial_port_base::parity::parity(serial_port_base::parity::type t)
  : value_(t)
{
  if (t != none && t != odd && t != even)
  {
    std::out_of_range ex("invalid parity value");
    boost::asio::detail::throw_exception(ex);
  }
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::parity::store(
    BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec) const
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  switch (value_)
  {
  case none:
    storage.fParity = FALSE;
    storage.Parity = NOPARITY;
    break;
  case odd:
    storage.fParity = TRUE;
    storage.Parity = ODDPARITY;
    break;
  case even:
    storage.fParity = TRUE;
    storage.Parity = EVENPARITY;
    break;
  default:
    break;
  }
#else
  switch (value_)
  {
  case none:
    storage.c_iflag |= IGNPAR;
    storage.c_cflag &= ~(PARENB | PARODD);
    break;
  case even:
    storage.c_iflag &= ~(IGNPAR | PARMRK);
    storage.c_iflag |= INPCK;
    storage.c_cflag |= PARENB;
    storage.c_cflag &= ~PARODD;
    break;
  case odd:
    storage.c_iflag &= ~(IGNPAR | PARMRK);
    storage.c_iflag |= INPCK;
    storage.c_cflag |= (PARENB | PARODD);
    break;
  default:
    break;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::parity::load(
    const BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec)
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  if (storage.Parity == EVENPARITY)
  {
    value_ = even;
  }
  else if (storage.Parity == ODDPARITY)
  {
    value_ = odd;
  }
  else
  {
    value_ = none;
  }
#else
  if (storage.c_cflag & PARENB)
  {
    if (storage.c_cflag & PARODD)
    {
      value_ = odd;
    }
    else
    {
      value_ = even;
    }
  }
  else
  {
    value_ = none;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

serial_port_base::stop_bits::stop_bits(
    serial_port_base::stop_bits::type t)
  : value_(t)
{
  if (t != one && t != onepointfive && t != two)
  {
    std::out_of_range ex("invalid stop_bits value");
    boost::asio::detail::throw_exception(ex);
  }
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::stop_bits::store(
    BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec) const
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  switch (value_)
  {
  case one:
    storage.StopBits = ONESTOPBIT;
    break;
  case onepointfive:
    storage.StopBits = ONE5STOPBITS;
    break;
  case two:
    storage.StopBits = TWOSTOPBITS;
    break;
  default:
    break;
  }
#else
  switch (value_)
  {
  case one:
    storage.c_cflag &= ~CSTOPB;
    break;
  case two:
    storage.c_cflag |= CSTOPB;
    break;
  default:
    ec = boost::asio::error::operation_not_supported;
    BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::stop_bits::load(
    const BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec)
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  if (storage.StopBits == ONESTOPBIT)
  {
    value_ = one;
  }
  else if (storage.StopBits == ONE5STOPBITS)
  {
    value_ = onepointfive;
  }
  else if (storage.StopBits == TWOSTOPBITS)
  {
    value_ = two;
  }
  else
  {
    value_ = one;
  }
#else
  value_ = (storage.c_cflag & CSTOPB) ? two : one;
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

serial_port_base::character_size::character_size(unsigned int t)
  : value_(t)
{
  if (t < 5 || t > 8)
  {
    std::out_of_range ex("invalid character_size value");
    boost::asio::detail::throw_exception(ex);
  }
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::character_size::store(
    BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec) const
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  storage.ByteSize = value_;
#else
  storage.c_cflag &= ~CSIZE;
  switch (value_)
  {
  case 5: storage.c_cflag |= CS5; break;
  case 6: storage.c_cflag |= CS6; break;
  case 7: storage.c_cflag |= CS7; break;
  case 8: storage.c_cflag |= CS8; break;
  default: break;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

BOOST_ASIO_SYNC_OP_VOID serial_port_base::character_size::load(
    const BOOST_ASIO_OPTION_STORAGE& storage, boost::system::error_code& ec)
{
#if defined(BOOST_ASIO_WINDOWS) || defined(__CYGWIN__)
  value_ = storage.ByteSize;
#else
  if ((storage.c_cflag & CSIZE) == CS5) { value_ = 5; }
  else if ((storage.c_cflag & CSIZE) == CS6) { value_ = 6; }
  else if ((storage.c_cflag & CSIZE) == CS7) { value_ = 7; }
  else if ((storage.c_cflag & CSIZE) == CS8) { value_ = 8; }
  else
  {
    // Hmmm, use 8 for now.
    value_ = 8;
  }
#endif
  ec = boost::system::error_code();
  BOOST_ASIO_SYNC_OP_VOID_RETURN(ec);
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#undef BOOST_ASIO_OPTION_STORAGE

#endif // defined(BOOST_ASIO_HAS_SERIAL_PORT)

#endif // BOOST_ASIO_IMPL_SERIAL_PORT_BASE_IPP
