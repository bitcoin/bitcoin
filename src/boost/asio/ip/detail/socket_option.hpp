//
// detail/socket_option.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_DETAIL_SOCKET_OPTION_HPP
#define BOOST_ASIO_IP_DETAIL_SOCKET_OPTION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/socket_types.hpp>
#include <boost/asio/detail/throw_exception.hpp>
#include <boost/asio/ip/address.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {
namespace detail {
namespace socket_option {

// Helper template for implementing multicast enable loopback options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class multicast_enable_loopback
{
public:
#if defined(__sun) || defined(__osf__)
  typedef unsigned char ipv4_value_type;
  typedef unsigned char ipv6_value_type;
#elif defined(_AIX) || defined(__hpux) || defined(__QNXNTO__) 
  typedef unsigned char ipv4_value_type;
  typedef unsigned int ipv6_value_type;
#else
  typedef int ipv4_value_type;
  typedef int ipv6_value_type;
#endif

  // Default constructor.
  multicast_enable_loopback()
    : ipv4_value_(0),
      ipv6_value_(0)
  {
  }

  // Construct with a specific option value.
  explicit multicast_enable_loopback(bool v)
    : ipv4_value_(v ? 1 : 0),
      ipv6_value_(v ? 1 : 0)
  {
  }

  // Set the value of the boolean.
  multicast_enable_loopback& operator=(bool v)
  {
    ipv4_value_ = v ? 1 : 0;
    ipv6_value_ = v ? 1 : 0;
    return *this;
  }

  // Get the current value of the boolean.
  bool value() const
  {
    return !!ipv4_value_;
  }

  // Convert to bool.
  operator bool() const
  {
    return !!ipv4_value_;
  }

  // Test for false.
  bool operator!() const
  {
    return !ipv4_value_;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the boolean data.
  template <typename Protocol>
  void* data(const Protocol& protocol)
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the address of the boolean data.
  template <typename Protocol>
  const void* data(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the size of the boolean data.
  template <typename Protocol>
  std::size_t size(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return sizeof(ipv6_value_);
    return sizeof(ipv4_value_);
  }

  // Set the size of the boolean data.
  template <typename Protocol>
  void resize(const Protocol& protocol, std::size_t s)
  {
    if (protocol.family() == PF_INET6)
    {
      if (s != sizeof(ipv6_value_))
      {
        std::length_error ex("multicast_enable_loopback socket option resize");
        boost::asio::detail::throw_exception(ex);
      }
      ipv4_value_ = ipv6_value_ ? 1 : 0;
    }
    else
    {
      if (s != sizeof(ipv4_value_))
      {
        std::length_error ex("multicast_enable_loopback socket option resize");
        boost::asio::detail::throw_exception(ex);
      }
      ipv6_value_ = ipv4_value_ ? 1 : 0;
    }
  }

private:
  ipv4_value_type ipv4_value_;
  ipv6_value_type ipv6_value_;
};

// Helper template for implementing unicast hops options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class unicast_hops
{
public:
  // Default constructor.
  unicast_hops()
    : value_(0)
  {
  }

  // Construct with a specific option value.
  explicit unicast_hops(int v)
    : value_(v)
  {
  }

  // Set the value of the option.
  unicast_hops& operator=(int v)
  {
    value_ = v;
    return *this;
  }

  // Get the current value of the option.
  int value() const
  {
    return value_;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the data.
  template <typename Protocol>
  int* data(const Protocol&)
  {
    return &value_;
  }

  // Get the address of the data.
  template <typename Protocol>
  const int* data(const Protocol&) const
  {
    return &value_;
  }

  // Get the size of the data.
  template <typename Protocol>
  std::size_t size(const Protocol&) const
  {
    return sizeof(value_);
  }

  // Set the size of the data.
  template <typename Protocol>
  void resize(const Protocol&, std::size_t s)
  {
    if (s != sizeof(value_))
    {
      std::length_error ex("unicast hops socket option resize");
      boost::asio::detail::throw_exception(ex);
    }
#if defined(__hpux)
    if (value_ < 0)
      value_ = value_ & 0xFF;
#endif
  }

private:
  int value_;
};

// Helper template for implementing multicast hops options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class multicast_hops
{
public:
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  typedef int ipv4_value_type;
#else
  typedef unsigned char ipv4_value_type;
#endif
  typedef int ipv6_value_type;

  // Default constructor.
  multicast_hops()
    : ipv4_value_(0),
      ipv6_value_(0)
  {
  }

  // Construct with a specific option value.
  explicit multicast_hops(int v)
  {
    if (v < 0 || v > 255)
    {
      std::out_of_range ex("multicast hops value out of range");
      boost::asio::detail::throw_exception(ex);
    }
    ipv4_value_ = (ipv4_value_type)v;
    ipv6_value_ = v;
  }

  // Set the value of the option.
  multicast_hops& operator=(int v)
  {
    if (v < 0 || v > 255)
    {
      std::out_of_range ex("multicast hops value out of range");
      boost::asio::detail::throw_exception(ex);
    }
    ipv4_value_ = (ipv4_value_type)v;
    ipv6_value_ = v;
    return *this;
  }

  // Get the current value of the option.
  int value() const
  {
    return ipv6_value_;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the data.
  template <typename Protocol>
  void* data(const Protocol& protocol)
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the address of the data.
  template <typename Protocol>
  const void* data(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the size of the data.
  template <typename Protocol>
  std::size_t size(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return sizeof(ipv6_value_);
    return sizeof(ipv4_value_);
  }

  // Set the size of the data.
  template <typename Protocol>
  void resize(const Protocol& protocol, std::size_t s)
  {
    if (protocol.family() == PF_INET6)
    {
      if (s != sizeof(ipv6_value_))
      {
        std::length_error ex("multicast hops socket option resize");
        boost::asio::detail::throw_exception(ex);
      }
      if (ipv6_value_ < 0)
        ipv4_value_ = 0;
      else if (ipv6_value_ > 255)
        ipv4_value_ = 255;
      else
        ipv4_value_ = (ipv4_value_type)ipv6_value_;
    }
    else
    {
      if (s != sizeof(ipv4_value_))
      {
        std::length_error ex("multicast hops socket option resize");
        boost::asio::detail::throw_exception(ex);
      }
      ipv6_value_ = ipv4_value_;
    }
  }

private:
  ipv4_value_type ipv4_value_;
  ipv6_value_type ipv6_value_;
};

// Helper template for implementing ip_mreq-based options.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class multicast_request
{
public:
  // Default constructor.
  multicast_request()
    : ipv4_value_(), // Zero-initialisation gives the "any" address.
      ipv6_value_() // Zero-initialisation gives the "any" address.
  {
  }

  // Construct with multicast address only.
  explicit multicast_request(const address& multicast_address)
    : ipv4_value_(), // Zero-initialisation gives the "any" address.
      ipv6_value_() // Zero-initialisation gives the "any" address.
  {
    if (multicast_address.is_v6())
    {
      using namespace std; // For memcpy.
      address_v6 ipv6_address = multicast_address.to_v6();
      address_v6::bytes_type bytes = ipv6_address.to_bytes();
      memcpy(ipv6_value_.ipv6mr_multiaddr.s6_addr, bytes.data(), 16);
      ipv6_value_.ipv6mr_interface = ipv6_address.scope_id();
    }
    else
    {
      ipv4_value_.imr_multiaddr.s_addr =
        boost::asio::detail::socket_ops::host_to_network_long(
            multicast_address.to_v4().to_uint());
      ipv4_value_.imr_interface.s_addr =
        boost::asio::detail::socket_ops::host_to_network_long(
            address_v4::any().to_uint());
    }
  }

  // Construct with multicast address and IPv4 address specifying an interface.
  explicit multicast_request(const address_v4& multicast_address,
      const address_v4& network_interface = address_v4::any())
    : ipv6_value_() // Zero-initialisation gives the "any" address.
  {
    ipv4_value_.imr_multiaddr.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
          multicast_address.to_uint());
    ipv4_value_.imr_interface.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
          network_interface.to_uint());
  }

  // Construct with multicast address and IPv6 network interface index.
  explicit multicast_request(
      const address_v6& multicast_address,
      unsigned long network_interface = 0)
    : ipv4_value_() // Zero-initialisation gives the "any" address.
  {
    using namespace std; // For memcpy.
    address_v6::bytes_type bytes = multicast_address.to_bytes();
    memcpy(ipv6_value_.ipv6mr_multiaddr.s6_addr, bytes.data(), 16);
    if (network_interface)
      ipv6_value_.ipv6mr_interface = network_interface;
    else
      ipv6_value_.ipv6mr_interface = multicast_address.scope_id();
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the option data.
  template <typename Protocol>
  const void* data(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the size of the option data.
  template <typename Protocol>
  std::size_t size(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return sizeof(ipv6_value_);
    return sizeof(ipv4_value_);
  }

private:
  boost::asio::detail::in4_mreq_type ipv4_value_;
  boost::asio::detail::in6_mreq_type ipv6_value_;
};

// Helper template for implementing options that specify a network interface.
template <int IPv4_Level, int IPv4_Name, int IPv6_Level, int IPv6_Name>
class network_interface
{
public:
  // Default constructor.
  network_interface()
  {
    ipv4_value_.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
          address_v4::any().to_uint());
    ipv6_value_ = 0;
  }

  // Construct with IPv4 interface.
  explicit network_interface(const address_v4& ipv4_interface)
  {
    ipv4_value_.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
          ipv4_interface.to_uint());
    ipv6_value_ = 0;
  }

  // Construct with IPv6 interface.
  explicit network_interface(unsigned int ipv6_interface)
  {
    ipv4_value_.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
          address_v4::any().to_uint());
    ipv6_value_ = ipv6_interface;
  }

  // Get the level of the socket option.
  template <typename Protocol>
  int level(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Level;
    return IPv4_Level;
  }

  // Get the name of the socket option.
  template <typename Protocol>
  int name(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return IPv6_Name;
    return IPv4_Name;
  }

  // Get the address of the option data.
  template <typename Protocol>
  const void* data(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return &ipv6_value_;
    return &ipv4_value_;
  }

  // Get the size of the option data.
  template <typename Protocol>
  std::size_t size(const Protocol& protocol) const
  {
    if (protocol.family() == PF_INET6)
      return sizeof(ipv6_value_);
    return sizeof(ipv4_value_);
  }

private:
  boost::asio::detail::in4_addr_type ipv4_value_;
  unsigned int ipv6_value_;
};

} // namespace socket_option
} // namespace detail
} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_DETAIL_SOCKET_OPTION_HPP
