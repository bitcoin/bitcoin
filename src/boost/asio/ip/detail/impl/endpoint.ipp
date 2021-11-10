//
// ip/detail/impl/endpoint.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_DETAIL_IMPL_ENDPOINT_IPP
#define BOOST_ASIO_IP_DETAIL_IMPL_ENDPOINT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstring>
#if !defined(BOOST_ASIO_NO_IOSTREAM)
# include <sstream>
#endif // !defined(BOOST_ASIO_NO_IOSTREAM)
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/detail/endpoint.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {
namespace detail {

endpoint::endpoint() BOOST_ASIO_NOEXCEPT
  : data_()
{
  data_.v4.sin_family = BOOST_ASIO_OS_DEF(AF_INET);
  data_.v4.sin_port = 0;
  data_.v4.sin_addr.s_addr = BOOST_ASIO_OS_DEF(INADDR_ANY);
}

endpoint::endpoint(int family, unsigned short port_num) BOOST_ASIO_NOEXCEPT
  : data_()
{
  using namespace std; // For memcpy.
  if (family == BOOST_ASIO_OS_DEF(AF_INET))
  {
    data_.v4.sin_family = BOOST_ASIO_OS_DEF(AF_INET);
    data_.v4.sin_port =
      boost::asio::detail::socket_ops::host_to_network_short(port_num);
    data_.v4.sin_addr.s_addr = BOOST_ASIO_OS_DEF(INADDR_ANY);
  }
  else
  {
    data_.v6.sin6_family = BOOST_ASIO_OS_DEF(AF_INET6);
    data_.v6.sin6_port =
      boost::asio::detail::socket_ops::host_to_network_short(port_num);
    data_.v6.sin6_flowinfo = 0;
    data_.v6.sin6_addr.s6_addr[0] = 0; data_.v6.sin6_addr.s6_addr[1] = 0;
    data_.v6.sin6_addr.s6_addr[2] = 0; data_.v6.sin6_addr.s6_addr[3] = 0;
    data_.v6.sin6_addr.s6_addr[4] = 0; data_.v6.sin6_addr.s6_addr[5] = 0;
    data_.v6.sin6_addr.s6_addr[6] = 0; data_.v6.sin6_addr.s6_addr[7] = 0;
    data_.v6.sin6_addr.s6_addr[8] = 0; data_.v6.sin6_addr.s6_addr[9] = 0;
    data_.v6.sin6_addr.s6_addr[10] = 0; data_.v6.sin6_addr.s6_addr[11] = 0;
    data_.v6.sin6_addr.s6_addr[12] = 0; data_.v6.sin6_addr.s6_addr[13] = 0;
    data_.v6.sin6_addr.s6_addr[14] = 0; data_.v6.sin6_addr.s6_addr[15] = 0;
    data_.v6.sin6_scope_id = 0;
  }
}

endpoint::endpoint(const boost::asio::ip::address& addr,
    unsigned short port_num) BOOST_ASIO_NOEXCEPT
  : data_()
{
  using namespace std; // For memcpy.
  if (addr.is_v4())
  {
    data_.v4.sin_family = BOOST_ASIO_OS_DEF(AF_INET);
    data_.v4.sin_port =
      boost::asio::detail::socket_ops::host_to_network_short(port_num);
    data_.v4.sin_addr.s_addr =
      boost::asio::detail::socket_ops::host_to_network_long(
        addr.to_v4().to_uint());
  }
  else
  {
    data_.v6.sin6_family = BOOST_ASIO_OS_DEF(AF_INET6);
    data_.v6.sin6_port =
      boost::asio::detail::socket_ops::host_to_network_short(port_num);
    data_.v6.sin6_flowinfo = 0;
    boost::asio::ip::address_v6 v6_addr = addr.to_v6();
    boost::asio::ip::address_v6::bytes_type bytes = v6_addr.to_bytes();
    memcpy(data_.v6.sin6_addr.s6_addr, bytes.data(), 16);
    data_.v6.sin6_scope_id =
      static_cast<boost::asio::detail::u_long_type>(
        v6_addr.scope_id());
  }
}

void endpoint::resize(std::size_t new_size)
{
  if (new_size > sizeof(boost::asio::detail::sockaddr_storage_type))
  {
    boost::system::error_code ec(boost::asio::error::invalid_argument);
    boost::asio::detail::throw_error(ec);
  }
}

unsigned short endpoint::port() const BOOST_ASIO_NOEXCEPT
{
  if (is_v4())
  {
    return boost::asio::detail::socket_ops::network_to_host_short(
        data_.v4.sin_port);
  }
  else
  {
    return boost::asio::detail::socket_ops::network_to_host_short(
        data_.v6.sin6_port);
  }
}

void endpoint::port(unsigned short port_num) BOOST_ASIO_NOEXCEPT
{
  if (is_v4())
  {
    data_.v4.sin_port
      = boost::asio::detail::socket_ops::host_to_network_short(port_num);
  }
  else
  {
    data_.v6.sin6_port
      = boost::asio::detail::socket_ops::host_to_network_short(port_num);
  }
}

boost::asio::ip::address endpoint::address() const BOOST_ASIO_NOEXCEPT
{
  using namespace std; // For memcpy.
  if (is_v4())
  {
    return boost::asio::ip::address_v4(
        boost::asio::detail::socket_ops::network_to_host_long(
          data_.v4.sin_addr.s_addr));
  }
  else
  {
    boost::asio::ip::address_v6::bytes_type bytes;
#if defined(BOOST_ASIO_HAS_STD_ARRAY)
    memcpy(bytes.data(), data_.v6.sin6_addr.s6_addr, 16);
#else // defined(BOOST_ASIO_HAS_STD_ARRAY)
    memcpy(bytes.elems, data_.v6.sin6_addr.s6_addr, 16);
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)
    return boost::asio::ip::address_v6(bytes, data_.v6.sin6_scope_id);
  }
}

void endpoint::address(const boost::asio::ip::address& addr) BOOST_ASIO_NOEXCEPT
{
  endpoint tmp_endpoint(addr, port());
  data_ = tmp_endpoint.data_;
}

bool operator==(const endpoint& e1, const endpoint& e2) BOOST_ASIO_NOEXCEPT
{
  return e1.address() == e2.address() && e1.port() == e2.port();
}

bool operator<(const endpoint& e1, const endpoint& e2) BOOST_ASIO_NOEXCEPT
{
  if (e1.address() < e2.address())
    return true;
  if (e1.address() != e2.address())
    return false;
  return e1.port() < e2.port();
}

#if !defined(BOOST_ASIO_NO_IOSTREAM)
std::string endpoint::to_string() const
{
  std::ostringstream tmp_os;
  tmp_os.imbue(std::locale::classic());
  if (is_v4())
    tmp_os << address();
  else
    tmp_os << '[' << address() << ']';
  tmp_os << ':' << port();

  return tmp_os.str();
}
#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

} // namespace detail
} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_DETAIL_IMPL_ENDPOINT_IPP
