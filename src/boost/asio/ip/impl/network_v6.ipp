//
// ip/impl/network_v6.ipp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2014 Oliver Kowalke (oliver dot kowalke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_IMPL_NETWORK_V6_IPP
#define BOOST_ASIO_IP_IMPL_NETWORK_V6_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <boost/asio/error.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/detail/throw_exception.hpp>
#include <boost/asio/ip/network_v6.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

network_v6::network_v6(const address_v6& addr, unsigned short prefix_len)
  : address_(addr),
    prefix_length_(prefix_len)
{
  if (prefix_len > 128)
  {
    std::out_of_range ex("prefix length too large");
    boost::asio::detail::throw_exception(ex);
  }
}

BOOST_ASIO_DECL address_v6 network_v6::network() const BOOST_ASIO_NOEXCEPT
{
  address_v6::bytes_type bytes(address_.to_bytes());
  for (std::size_t i = 0; i < 16; ++i)
  {
    if (prefix_length_ <= i * 8)
      bytes[i] = 0;
    else if (prefix_length_ < (i + 1) * 8)
      bytes[i] &= 0xFF00 >> (prefix_length_ % 8);
  }
  return address_v6(bytes, address_.scope_id());
}

address_v6_range network_v6::hosts() const BOOST_ASIO_NOEXCEPT
{
  address_v6::bytes_type begin_bytes(address_.to_bytes());
  address_v6::bytes_type end_bytes(address_.to_bytes());
  for (std::size_t i = 0; i < 16; ++i)
  {
    if (prefix_length_ <= i * 8)
    {
      begin_bytes[i] = 0;
      end_bytes[i] = 0xFF;
    }
    else if (prefix_length_ < (i + 1) * 8)
    {
      begin_bytes[i] &= 0xFF00 >> (prefix_length_ % 8);
      end_bytes[i] |= 0xFF >> (prefix_length_ % 8);
    }
  }
  return address_v6_range(
      address_v6_iterator(address_v6(begin_bytes, address_.scope_id())),
      ++address_v6_iterator(address_v6(end_bytes, address_.scope_id())));
}

bool network_v6::is_subnet_of(const network_v6& other) const
{
  if (other.prefix_length_ >= prefix_length_)
    return false; // Only real subsets are allowed.
  const network_v6 me(address_, other.prefix_length_);
  return other.canonical() == me.canonical();
}

std::string network_v6::to_string() const
{
  boost::system::error_code ec;
  std::string addr = to_string(ec);
  boost::asio::detail::throw_error(ec);
  return addr;
}

std::string network_v6::to_string(boost::system::error_code& ec) const
{
  using namespace std; // For sprintf.
  ec = boost::system::error_code();
  char prefix_len[16];
#if defined(BOOST_ASIO_HAS_SECURE_RTL)
  sprintf_s(prefix_len, sizeof(prefix_len), "/%u", prefix_length_);
#else // defined(BOOST_ASIO_HAS_SECURE_RTL)
  sprintf(prefix_len, "/%u", prefix_length_);
#endif // defined(BOOST_ASIO_HAS_SECURE_RTL)
  return address_.to_string() + prefix_len;
}

network_v6 make_network_v6(const char* str)
{
  return make_network_v6(std::string(str));
}

network_v6 make_network_v6(const char* str, boost::system::error_code& ec)
{
  return make_network_v6(std::string(str), ec);
}

network_v6 make_network_v6(const std::string& str)
{
  boost::system::error_code ec;
  network_v6 net = make_network_v6(str, ec);
  boost::asio::detail::throw_error(ec);
  return net;
}

network_v6 make_network_v6(const std::string& str,
    boost::system::error_code& ec)
{
  std::string::size_type pos = str.find_first_of("/");

  if (pos == std::string::npos)
  {
    ec = boost::asio::error::invalid_argument;
    return network_v6();
  }

  if (pos == str.size() - 1)
  {
    ec = boost::asio::error::invalid_argument;
    return network_v6();
  }

  std::string::size_type end = str.find_first_not_of("0123456789", pos + 1);
  if (end != std::string::npos)
  {
    ec = boost::asio::error::invalid_argument;
    return network_v6();
  }

  const address_v6 addr = make_address_v6(str.substr(0, pos), ec);
  if (ec)
    return network_v6();

  const int prefix_len = std::atoi(str.substr(pos + 1).c_str());
  if (prefix_len < 0 || prefix_len > 128)
  {
    ec = boost::asio::error::invalid_argument;
    return network_v6();
  }

  return network_v6(addr, static_cast<unsigned short>(prefix_len));
}

#if defined(BOOST_ASIO_HAS_STRING_VIEW)

network_v6 make_network_v6(string_view str)
{
  return make_network_v6(static_cast<std::string>(str));
}

network_v6 make_network_v6(string_view str,
    boost::system::error_code& ec)
{
  return make_network_v6(static_cast<std::string>(str), ec);
}

#endif // defined(BOOST_ASIO_HAS_STRING_VIEW)

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_IMPL_NETWORK_V6_IPP
