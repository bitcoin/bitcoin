//
// ip/basic_endpoint.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_BASIC_ENDPOINT_HPP
#define BOOST_ASIO_IP_BASIC_ENDPOINT_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/cstdint.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/detail/endpoint.hpp>

#if defined(BOOST_ASIO_HAS_STD_HASH)
# include <functional>
#endif // defined(BOOST_ASIO_HAS_STD_HASH)

#if !defined(BOOST_ASIO_NO_IOSTREAM)
# include <iosfwd>
#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

/// Type used for storing port numbers.
typedef uint_least16_t port_type;

/// Describes an endpoint for a version-independent IP socket.
/**
 * The boost::asio::ip::basic_endpoint class template describes an endpoint that
 * may be associated with a particular socket.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 *
 * @par Concepts:
 * Endpoint.
 */
template <typename InternetProtocol>
class basic_endpoint
{
public:
  /// The protocol type associated with the endpoint.
  typedef InternetProtocol protocol_type;

  /// The type of the endpoint structure. This type is dependent on the
  /// underlying implementation of the socket layer.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined data_type;
#else
  typedef boost::asio::detail::socket_addr_type data_type;
#endif

  /// Default constructor.
  basic_endpoint() BOOST_ASIO_NOEXCEPT
    : impl_()
  {
  }

  /// Construct an endpoint using a port number, specified in the host's byte
  /// order. The IP address will be the any address (i.e. INADDR_ANY or
  /// in6addr_any). This constructor would typically be used for accepting new
  /// connections.
  /**
   * @par Examples
   * To initialise an IPv4 TCP endpoint for port 1234, use:
   * @code
   * boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 1234);
   * @endcode
   *
   * To specify an IPv6 UDP endpoint for port 9876, use:
   * @code
   * boost::asio::ip::udp::endpoint ep(boost::asio::ip::udp::v6(), 9876);
   * @endcode
   */
  basic_endpoint(const InternetProtocol& internet_protocol,
      port_type port_num) BOOST_ASIO_NOEXCEPT
    : impl_(internet_protocol.family(), port_num)
  {
  }

  /// Construct an endpoint using a port number and an IP address. This
  /// constructor may be used for accepting connections on a specific interface
  /// or for making a connection to a remote endpoint.
  basic_endpoint(const boost::asio::ip::address& addr,
      port_type port_num) BOOST_ASIO_NOEXCEPT
    : impl_(addr, port_num)
  {
  }

  /// Copy constructor.
  basic_endpoint(const basic_endpoint& other) BOOST_ASIO_NOEXCEPT
    : impl_(other.impl_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move constructor.
  basic_endpoint(basic_endpoint&& other) BOOST_ASIO_NOEXCEPT
    : impl_(other.impl_)
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Assign from another endpoint.
  basic_endpoint& operator=(const basic_endpoint& other) BOOST_ASIO_NOEXCEPT
  {
    impl_ = other.impl_;
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move-assign from another endpoint.
  basic_endpoint& operator=(basic_endpoint&& other) BOOST_ASIO_NOEXCEPT
  {
    impl_ = other.impl_;
    return *this;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// The protocol associated with the endpoint.
  protocol_type protocol() const BOOST_ASIO_NOEXCEPT
  {
    if (impl_.is_v4())
      return InternetProtocol::v4();
    return InternetProtocol::v6();
  }

  /// Get the underlying endpoint in the native type.
  data_type* data() BOOST_ASIO_NOEXCEPT
  {
    return impl_.data();
  }

  /// Get the underlying endpoint in the native type.
  const data_type* data() const BOOST_ASIO_NOEXCEPT
  {
    return impl_.data();
  }

  /// Get the underlying size of the endpoint in the native type.
  std::size_t size() const BOOST_ASIO_NOEXCEPT
  {
    return impl_.size();
  }

  /// Set the underlying size of the endpoint in the native type.
  void resize(std::size_t new_size)
  {
    impl_.resize(new_size);
  }

  /// Get the capacity of the endpoint in the native type.
  std::size_t capacity() const BOOST_ASIO_NOEXCEPT
  {
    return impl_.capacity();
  }

  /// Get the port associated with the endpoint. The port number is always in
  /// the host's byte order.
  port_type port() const BOOST_ASIO_NOEXCEPT
  {
    return impl_.port();
  }

  /// Set the port associated with the endpoint. The port number is always in
  /// the host's byte order.
  void port(port_type port_num) BOOST_ASIO_NOEXCEPT
  {
    impl_.port(port_num);
  }

  /// Get the IP address associated with the endpoint.
  boost::asio::ip::address address() const BOOST_ASIO_NOEXCEPT
  {
    return impl_.address();
  }

  /// Set the IP address associated with the endpoint.
  void address(const boost::asio::ip::address& addr) BOOST_ASIO_NOEXCEPT
  {
    impl_.address(addr);
  }

  /// Compare two endpoints for equality.
  friend bool operator==(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return e1.impl_ == e2.impl_;
  }

  /// Compare two endpoints for inequality.
  friend bool operator!=(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return !(e1 == e2);
  }

  /// Compare endpoints for ordering.
  friend bool operator<(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return e1.impl_ < e2.impl_;
  }

  /// Compare endpoints for ordering.
  friend bool operator>(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return e2.impl_ < e1.impl_;
  }

  /// Compare endpoints for ordering.
  friend bool operator<=(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return !(e2 < e1);
  }

  /// Compare endpoints for ordering.
  friend bool operator>=(const basic_endpoint<InternetProtocol>& e1,
      const basic_endpoint<InternetProtocol>& e2) BOOST_ASIO_NOEXCEPT
  {
    return !(e1 < e2);
  }

private:
  // The underlying IP endpoint.
  boost::asio::ip::detail::endpoint impl_;
};

#if !defined(BOOST_ASIO_NO_IOSTREAM)

/// Output an endpoint as a string.
/**
 * Used to output a human-readable string for a specified endpoint.
 *
 * @param os The output stream to which the string will be written.
 *
 * @param endpoint The endpoint to be written.
 *
 * @return The output stream.
 *
 * @relates boost::asio::ip::basic_endpoint
 */
template <typename Elem, typename Traits, typename InternetProtocol>
std::basic_ostream<Elem, Traits>& operator<<(
    std::basic_ostream<Elem, Traits>& os,
    const basic_endpoint<InternetProtocol>& endpoint);

#endif // !defined(BOOST_ASIO_NO_IOSTREAM)

} // namespace ip
} // namespace asio
} // namespace boost

#if defined(BOOST_ASIO_HAS_STD_HASH)
namespace std {

template <typename InternetProtocol>
struct hash<boost::asio::ip::basic_endpoint<InternetProtocol> >
{
  std::size_t operator()(
      const boost::asio::ip::basic_endpoint<InternetProtocol>& ep)
    const BOOST_ASIO_NOEXCEPT
  {
    std::size_t hash1 = std::hash<boost::asio::ip::address>()(ep.address());
    std::size_t hash2 = std::hash<unsigned short>()(ep.port());
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
  }
};

} // namespace std
#endif // defined(BOOST_ASIO_HAS_STD_HASH)

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/ip/impl/basic_endpoint.hpp>

#endif // BOOST_ASIO_IP_BASIC_ENDPOINT_HPP
