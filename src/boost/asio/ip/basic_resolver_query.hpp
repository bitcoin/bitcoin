//
// ip/basic_resolver_query.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_BASIC_RESOLVER_QUERY_HPP
#define BOOST_ASIO_IP_BASIC_RESOLVER_QUERY_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <string>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/ip/resolver_query_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

/// An query to be passed to a resolver.
/**
 * The boost::asio::ip::basic_resolver_query class template describes a query
 * that can be passed to a resolver.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename InternetProtocol>
class basic_resolver_query
  : public resolver_query_base
{
public:
  /// The protocol type associated with the endpoint query.
  typedef InternetProtocol protocol_type;

  /// Construct with specified service name for any protocol.
  /**
   * This constructor is typically used to perform name resolution for local
   * service binding.
   *
   * @param service A string identifying the requested service. This may be a
   * descriptive name or a numeric string corresponding to a port number.
   *
   * @param resolve_flags A set of flags that determine how name resolution
   * should be performed. The default flags are suitable for local service
   * binding.
   *
   * @note On POSIX systems, service names are typically defined in the file
   * <tt>/etc/services</tt>. On Windows, service names may be found in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\services</tt>. Operating systems
   * may use additional locations when resolving service names.
   */
  basic_resolver_query(const std::string& service,
      resolver_query_base::flags resolve_flags = passive | address_configured)
    : hints_(),
      host_name_(),
      service_name_(service)
  {
    typename InternetProtocol::endpoint endpoint;
    hints_.ai_flags = static_cast<int>(resolve_flags);
    hints_.ai_family = PF_UNSPEC;
    hints_.ai_socktype = endpoint.protocol().type();
    hints_.ai_protocol = endpoint.protocol().protocol();
    hints_.ai_addrlen = 0;
    hints_.ai_canonname = 0;
    hints_.ai_addr = 0;
    hints_.ai_next = 0;
  }

  /// Construct with specified service name for a given protocol.
  /**
   * This constructor is typically used to perform name resolution for local
   * service binding with a specific protocol version.
   *
   * @param protocol A protocol object, normally representing either the IPv4 or
   * IPv6 version of an internet protocol.
   *
   * @param service A string identifying the requested service. This may be a
   * descriptive name or a numeric string corresponding to a port number.
   *
   * @param resolve_flags A set of flags that determine how name resolution
   * should be performed. The default flags are suitable for local service
   * binding.
   *
   * @note On POSIX systems, service names are typically defined in the file
   * <tt>/etc/services</tt>. On Windows, service names may be found in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\services</tt>. Operating systems
   * may use additional locations when resolving service names.
   */
  basic_resolver_query(const protocol_type& protocol,
      const std::string& service,
      resolver_query_base::flags resolve_flags = passive | address_configured)
    : hints_(),
      host_name_(),
      service_name_(service)
  {
    hints_.ai_flags = static_cast<int>(resolve_flags);
    hints_.ai_family = protocol.family();
    hints_.ai_socktype = protocol.type();
    hints_.ai_protocol = protocol.protocol();
    hints_.ai_addrlen = 0;
    hints_.ai_canonname = 0;
    hints_.ai_addr = 0;
    hints_.ai_next = 0;
  }

  /// Construct with specified host name and service name for any protocol.
  /**
   * This constructor is typically used to perform name resolution for
   * communication with remote hosts.
   *
   * @param host A string identifying a location. May be a descriptive name or
   * a numeric address string. If an empty string and the passive flag has been
   * specified, the resolved endpoints are suitable for local service binding.
   * If an empty string and passive is not specified, the resolved endpoints
   * will use the loopback address.
   *
   * @param service A string identifying the requested service. This may be a
   * descriptive name or a numeric string corresponding to a port number. May
   * be an empty string, in which case all resolved endpoints will have a port
   * number of 0.
   *
   * @param resolve_flags A set of flags that determine how name resolution
   * should be performed. The default flags are suitable for communication with
   * remote hosts.
   *
   * @note On POSIX systems, host names may be locally defined in the file
   * <tt>/etc/hosts</tt>. On Windows, host names may be defined in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\hosts</tt>. Remote host name
   * resolution is performed using DNS. Operating systems may use additional
   * locations when resolving host names (such as NETBIOS names on Windows).
   *
   * On POSIX systems, service names are typically defined in the file
   * <tt>/etc/services</tt>. On Windows, service names may be found in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\services</tt>. Operating systems
   * may use additional locations when resolving service names.
   */
  basic_resolver_query(const std::string& host, const std::string& service,
      resolver_query_base::flags resolve_flags = address_configured)
    : hints_(),
      host_name_(host),
      service_name_(service)
  {
    typename InternetProtocol::endpoint endpoint;
    hints_.ai_flags = static_cast<int>(resolve_flags);
    hints_.ai_family = BOOST_ASIO_OS_DEF(AF_UNSPEC);
    hints_.ai_socktype = endpoint.protocol().type();
    hints_.ai_protocol = endpoint.protocol().protocol();
    hints_.ai_addrlen = 0;
    hints_.ai_canonname = 0;
    hints_.ai_addr = 0;
    hints_.ai_next = 0;
  }

  /// Construct with specified host name and service name for a given protocol.
  /**
   * This constructor is typically used to perform name resolution for
   * communication with remote hosts.
   *
   * @param protocol A protocol object, normally representing either the IPv4 or
   * IPv6 version of an internet protocol.
   *
   * @param host A string identifying a location. May be a descriptive name or
   * a numeric address string. If an empty string and the passive flag has been
   * specified, the resolved endpoints are suitable for local service binding.
   * If an empty string and passive is not specified, the resolved endpoints
   * will use the loopback address.
   *
   * @param service A string identifying the requested service. This may be a
   * descriptive name or a numeric string corresponding to a port number. May
   * be an empty string, in which case all resolved endpoints will have a port
   * number of 0.
   *
   * @param resolve_flags A set of flags that determine how name resolution
   * should be performed. The default flags are suitable for communication with
   * remote hosts.
   *
   * @note On POSIX systems, host names may be locally defined in the file
   * <tt>/etc/hosts</tt>. On Windows, host names may be defined in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\hosts</tt>. Remote host name
   * resolution is performed using DNS. Operating systems may use additional
   * locations when resolving host names (such as NETBIOS names on Windows).
   *
   * On POSIX systems, service names are typically defined in the file
   * <tt>/etc/services</tt>. On Windows, service names may be found in the file
   * <tt>c:\\windows\\system32\\drivers\\etc\\services</tt>. Operating systems
   * may use additional locations when resolving service names.
   */
  basic_resolver_query(const protocol_type& protocol,
      const std::string& host, const std::string& service,
      resolver_query_base::flags resolve_flags = address_configured)
    : hints_(),
      host_name_(host),
      service_name_(service)
  {
    hints_.ai_flags = static_cast<int>(resolve_flags);
    hints_.ai_family = protocol.family();
    hints_.ai_socktype = protocol.type();
    hints_.ai_protocol = protocol.protocol();
    hints_.ai_addrlen = 0;
    hints_.ai_canonname = 0;
    hints_.ai_addr = 0;
    hints_.ai_next = 0;
  }

  /// Get the hints associated with the query.
  const boost::asio::detail::addrinfo_type& hints() const
  {
    return hints_;
  }

  /// Get the host name associated with the query.
  std::string host_name() const
  {
    return host_name_;
  }

  /// Get the service name associated with the query.
  std::string service_name() const
  {
    return service_name_;
  }

private:
  boost::asio::detail::addrinfo_type hints_;
  std::string host_name_;
  std::string service_name_;
};

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_BASIC_RESOLVER_QUERY_HPP
