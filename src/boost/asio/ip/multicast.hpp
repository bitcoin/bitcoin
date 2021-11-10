//
// ip/multicast.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_MULTICAST_HPP
#define BOOST_ASIO_IP_MULTICAST_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <boost/asio/ip/detail/socket_option.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {
namespace multicast {

/// Socket option to join a multicast group on a specified interface.
/**
 * Implements the IPPROTO_IP/IP_ADD_MEMBERSHIP socket option.
 *
 * @par Examples
 * Setting the option to join a multicast group:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::address multicast_address =
 *   boost::asio::ip::address::from_string("225.0.0.1");
 * boost::asio::ip::multicast::join_group option(multicast_address);
 * socket.set_option(option);
 * @endcode
 *
 * @par Concepts:
 * SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined join_group;
#else
typedef boost::asio::ip::detail::socket_option::multicast_request<
  BOOST_ASIO_OS_DEF(IPPROTO_IP),
  BOOST_ASIO_OS_DEF(IP_ADD_MEMBERSHIP),
  BOOST_ASIO_OS_DEF(IPPROTO_IPV6),
  BOOST_ASIO_OS_DEF(IPV6_JOIN_GROUP)> join_group;
#endif

/// Socket option to leave a multicast group on a specified interface.
/**
 * Implements the IPPROTO_IP/IP_DROP_MEMBERSHIP socket option.
 *
 * @par Examples
 * Setting the option to leave a multicast group:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::address multicast_address =
 *   boost::asio::ip::address::from_string("225.0.0.1");
 * boost::asio::ip::multicast::leave_group option(multicast_address);
 * socket.set_option(option);
 * @endcode
 *
 * @par Concepts:
 * SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined leave_group;
#else
typedef boost::asio::ip::detail::socket_option::multicast_request<
  BOOST_ASIO_OS_DEF(IPPROTO_IP),
  BOOST_ASIO_OS_DEF(IP_DROP_MEMBERSHIP),
  BOOST_ASIO_OS_DEF(IPPROTO_IPV6),
  BOOST_ASIO_OS_DEF(IPV6_LEAVE_GROUP)> leave_group;
#endif

/// Socket option for local interface to use for outgoing multicast packets.
/**
 * Implements the IPPROTO_IP/IP_MULTICAST_IF socket option.
 *
 * @par Examples
 * Setting the option:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::address_v4 local_interface =
 *   boost::asio::ip::address_v4::from_string("1.2.3.4");
 * boost::asio::ip::multicast::outbound_interface option(local_interface);
 * socket.set_option(option);
 * @endcode
 *
 * @par Concepts:
 * SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined outbound_interface;
#else
typedef boost::asio::ip::detail::socket_option::network_interface<
  BOOST_ASIO_OS_DEF(IPPROTO_IP),
  BOOST_ASIO_OS_DEF(IP_MULTICAST_IF),
  BOOST_ASIO_OS_DEF(IPPROTO_IPV6),
  BOOST_ASIO_OS_DEF(IPV6_MULTICAST_IF)> outbound_interface;
#endif

/// Socket option for time-to-live associated with outgoing multicast packets.
/**
 * Implements the IPPROTO_IP/IP_MULTICAST_TTL socket option.
 *
 * @par Examples
 * Setting the option:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::multicast::hops option(4);
 * socket.set_option(option);
 * @endcode
 *
 * @par
 * Getting the current option value:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::multicast::hops option;
 * socket.get_option(option);
 * int ttl = option.value();
 * @endcode
 *
 * @par Concepts:
 * GettableSocketOption, SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined hops;
#else
typedef boost::asio::ip::detail::socket_option::multicast_hops<
  BOOST_ASIO_OS_DEF(IPPROTO_IP),
  BOOST_ASIO_OS_DEF(IP_MULTICAST_TTL),
  BOOST_ASIO_OS_DEF(IPPROTO_IPV6),
  BOOST_ASIO_OS_DEF(IPV6_MULTICAST_HOPS)> hops;
#endif

/// Socket option determining whether outgoing multicast packets will be
/// received on the same socket if it is a member of the multicast group.
/**
 * Implements the IPPROTO_IP/IP_MULTICAST_LOOP socket option.
 *
 * @par Examples
 * Setting the option:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::multicast::enable_loopback option(true);
 * socket.set_option(option);
 * @endcode
 *
 * @par
 * Getting the current option value:
 * @code
 * boost::asio::ip::udp::socket socket(my_context);
 * ...
 * boost::asio::ip::multicast::enable_loopback option;
 * socket.get_option(option);
 * bool is_set = option.value();
 * @endcode
 *
 * @par Concepts:
 * GettableSocketOption, SettableSocketOption.
 */
#if defined(GENERATING_DOCUMENTATION)
typedef implementation_defined enable_loopback;
#else
typedef boost::asio::ip::detail::socket_option::multicast_enable_loopback<
  BOOST_ASIO_OS_DEF(IPPROTO_IP),
  BOOST_ASIO_OS_DEF(IP_MULTICAST_LOOP),
  BOOST_ASIO_OS_DEF(IPPROTO_IPV6),
  BOOST_ASIO_OS_DEF(IPV6_MULTICAST_LOOP)> enable_loopback;
#endif

} // namespace multicast
} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_MULTICAST_HPP
