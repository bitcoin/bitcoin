//
// impl/serial_port_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_SERIAL_PORT_BASE_HPP
#define BOOST_ASIO_IMPL_SERIAL_PORT_BASE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

inline serial_port_base::baud_rate::baud_rate(unsigned int rate)
  : value_(rate)
{
}

inline unsigned int serial_port_base::baud_rate::value() const
{
  return value_;
}

inline serial_port_base::flow_control::type
serial_port_base::flow_control::value() const
{
  return value_;
}

inline serial_port_base::parity::type serial_port_base::parity::value() const
{
  return value_;
}

inline serial_port_base::stop_bits::type
serial_port_base::stop_bits::value() const
{
  return value_;
}

inline unsigned int serial_port_base::character_size::value() const
{
  return value_;
}

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_SERIAL_PORT_BASE_HPP
