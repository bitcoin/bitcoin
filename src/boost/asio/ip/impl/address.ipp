//
// ip/impl/address.ipp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_IMPL_ADDRESS_IPP
#define BOOST_ASIO_IP_IMPL_ADDRESS_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <typeinfo>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/detail/throw_exception.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/bad_address_cast.hpp>
#include <boost/system/system_error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

address::address() BOOST_ASIO_NOEXCEPT
  : type_(ipv4),
    ipv4_address_(),
    ipv6_address_()
{
}

address::address(
    const boost::asio::ip::address_v4& ipv4_address) BOOST_ASIO_NOEXCEPT
  : type_(ipv4),
    ipv4_address_(ipv4_address),
    ipv6_address_()
{
}

address::address(
    const boost::asio::ip::address_v6& ipv6_address) BOOST_ASIO_NOEXCEPT
  : type_(ipv6),
    ipv4_address_(),
    ipv6_address_(ipv6_address)
{
}

address::address(const address& other) BOOST_ASIO_NOEXCEPT
  : type_(other.type_),
    ipv4_address_(other.ipv4_address_),
    ipv6_address_(other.ipv6_address_)
{
}

#if defined(BOOST_ASIO_HAS_MOVE)
address::address(address&& other) BOOST_ASIO_NOEXCEPT
  : type_(other.type_),
    ipv4_address_(other.ipv4_address_),
    ipv6_address_(other.ipv6_address_)
{
}
#endif // defined(BOOST_ASIO_HAS_MOVE)

address& address::operator=(const address& other) BOOST_ASIO_NOEXCEPT
{
  type_ = other.type_;
  ipv4_address_ = other.ipv4_address_;
  ipv6_address_ = other.ipv6_address_;
  return *this;
}

#if defined(BOOST_ASIO_HAS_MOVE)
address& address::operator=(address&& other) BOOST_ASIO_NOEXCEPT
{
  type_ = other.type_;
  ipv4_address_ = other.ipv4_address_;
  ipv6_address_ = other.ipv6_address_;
  return *this;
}
#endif // defined(BOOST_ASIO_HAS_MOVE)

address& address::operator=(
    const boost::asio::ip::address_v4& ipv4_address) BOOST_ASIO_NOEXCEPT
{
  type_ = ipv4;
  ipv4_address_ = ipv4_address;
  ipv6_address_ = boost::asio::ip::address_v6();
  return *this;
}

address& address::operator=(
    const boost::asio::ip::address_v6& ipv6_address) BOOST_ASIO_NOEXCEPT
{
  type_ = ipv6;
  ipv4_address_ = boost::asio::ip::address_v4();
  ipv6_address_ = ipv6_address;
  return *this;
}

address make_address(const char* str)
{
  boost::system::error_code ec;
  address addr = make_address(str, ec);
  boost::asio::detail::throw_error(ec);
  return addr;
}

address make_address(const char* str,
    boost::system::error_code& ec) BOOST_ASIO_NOEXCEPT
{
  boost::asio::ip::address_v6 ipv6_address =
    boost::asio::ip::make_address_v6(str, ec);
  if (!ec)
    return address(ipv6_address);

  boost::asio::ip::address_v4 ipv4_address =
    boost::asio::ip::make_address_v4(str, ec);
  if (!ec)
    return address(ipv4_address);

  return address();
}

address make_address(const std::string& str)
{
  return make_address(str.c_str());
}

address make_address(const std::string& str,
    boost::system::error_code& ec) BOOST_ASIO_NOEXCEPT
{
  return make_address(str.c_str(), ec);
}

#if defined(BOOST_ASIO_HAS_STRING_VIEW)

address make_address(string_view str)
{
  return make_address(static_cast<std::string>(str));
}

address make_address(string_view str,
    boost::system::error_code& ec) BOOST_ASIO_NOEXCEPT
{
  return make_address(static_cast<std::string>(str), ec);
}

#endif // defined(BOOST_ASIO_HAS_STRING_VIEW)

boost::asio::ip::address_v4 address::to_v4() const
{
  if (type_ != ipv4)
  {
    bad_address_cast ex;
    boost::asio::detail::throw_exception(ex);
  }
  return ipv4_address_;
}

boost::asio::ip::address_v6 address::to_v6() const
{
  if (type_ != ipv6)
  {
    bad_address_cast ex;
    boost::asio::detail::throw_exception(ex);
  }
  return ipv6_address_;
}

std::string address::to_string() const
{
  if (type_ == ipv6)
    return ipv6_address_.to_string();
  return ipv4_address_.to_string();
}

#if !defined(BOOST_ASIO_NO_DEPRECATED)
std::string address::to_string(boost::system::error_code& ec) const
{
  if (type_ == ipv6)
    return ipv6_address_.to_string(ec);
  return ipv4_address_.to_string(ec);
}
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)

bool address::is_loopback() const BOOST_ASIO_NOEXCEPT
{
  return (type_ == ipv4)
    ? ipv4_address_.is_loopback()
    : ipv6_address_.is_loopback();
}

bool address::is_unspecified() const BOOST_ASIO_NOEXCEPT
{
  return (type_ == ipv4)
    ? ipv4_address_.is_unspecified()
    : ipv6_address_.is_unspecified();
}

bool address::is_multicast() const BOOST_ASIO_NOEXCEPT
{
  return (type_ == ipv4)
    ? ipv4_address_.is_multicast()
    : ipv6_address_.is_multicast();
}

bool operator==(const address& a1, const address& a2) BOOST_ASIO_NOEXCEPT
{
  if (a1.type_ != a2.type_)
    return false;
  if (a1.type_ == address::ipv6)
    return a1.ipv6_address_ == a2.ipv6_address_;
  return a1.ipv4_address_ == a2.ipv4_address_;
}

bool operator<(const address& a1, const address& a2) BOOST_ASIO_NOEXCEPT
{
  if (a1.type_ < a2.type_)
    return true;
  if (a1.type_ > a2.type_)
    return false;
  if (a1.type_ == address::ipv6)
    return a1.ipv6_address_ < a2.ipv6_address_;
  return a1.ipv4_address_ < a2.ipv4_address_;
}

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_IMPL_ADDRESS_IPP
