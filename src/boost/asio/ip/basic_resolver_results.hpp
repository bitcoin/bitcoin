//
// ip/basic_resolver_results.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IP_BASIC_RESOLVER_RESULTS_HPP
#define BOOST_ASIO_IP_BASIC_RESOLVER_RESULTS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <cstddef>
#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/socket_types.hpp>
#include <boost/asio/ip/basic_resolver_iterator.hpp>

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)
# include <boost/asio/detail/winrt_utils.hpp>
#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace ip {

/// A range of entries produced by a resolver.
/**
 * The boost::asio::ip::basic_resolver_results class template is used to define
 * a range over the results returned by a resolver.
 *
 * The iterator's value_type, obtained when a results iterator is dereferenced,
 * is: @code const basic_resolver_entry<InternetProtocol> @endcode
 *
 * @note For backward compatibility, basic_resolver_results is derived from
 * basic_resolver_iterator. This derivation is deprecated.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename InternetProtocol>
class basic_resolver_results
#if !defined(BOOST_ASIO_NO_DEPRECATED)
  : public basic_resolver_iterator<InternetProtocol>
#else // !defined(BOOST_ASIO_NO_DEPRECATED)
  : private basic_resolver_iterator<InternetProtocol>
#endif // !defined(BOOST_ASIO_NO_DEPRECATED)
{
public:
  /// The protocol type associated with the results.
  typedef InternetProtocol protocol_type;

  /// The endpoint type associated with the results.
  typedef typename protocol_type::endpoint endpoint_type;

  /// The type of a value in the results range.
  typedef basic_resolver_entry<protocol_type> value_type;

  /// The type of a const reference to a value in the range.
  typedef const value_type& const_reference;

  /// The type of a non-const reference to a value in the range.
  typedef value_type& reference;

  /// The type of an iterator into the range.
  typedef basic_resolver_iterator<protocol_type> const_iterator;

  /// The type of an iterator into the range.
  typedef const_iterator iterator;

  /// Type used to represent the distance between two iterators in the range.
  typedef std::ptrdiff_t difference_type;

  /// Type used to represent a count of the elements in the range.
  typedef std::size_t size_type;

  /// Default constructor creates an empty range.
  basic_resolver_results()
  {
  }

  /// Copy constructor.
  basic_resolver_results(const basic_resolver_results& other)
    : basic_resolver_iterator<InternetProtocol>(other)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move constructor.
  basic_resolver_results(basic_resolver_results&& other)
    : basic_resolver_iterator<InternetProtocol>(
        BOOST_ASIO_MOVE_CAST(basic_resolver_results)(other))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Assignment operator.
  basic_resolver_results& operator=(const basic_resolver_results& other)
  {
    basic_resolver_iterator<InternetProtocol>::operator=(other);
    return *this;
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move-assignment operator.
  basic_resolver_results& operator=(basic_resolver_results&& other)
  {
    basic_resolver_iterator<InternetProtocol>::operator=(
        BOOST_ASIO_MOVE_CAST(basic_resolver_results)(other));
    return *this;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

#if !defined(GENERATING_DOCUMENTATION)
  // Create results from an addrinfo list returned by getaddrinfo.
  static basic_resolver_results create(
      boost::asio::detail::addrinfo_type* address_info,
      const std::string& host_name, const std::string& service_name)
  {
    basic_resolver_results results;
    if (!address_info)
      return results;

    std::string actual_host_name = host_name;
    if (address_info->ai_canonname)
      actual_host_name = address_info->ai_canonname;

    results.values_.reset(new values_type);

    while (address_info)
    {
      if (address_info->ai_family == BOOST_ASIO_OS_DEF(AF_INET)
          || address_info->ai_family == BOOST_ASIO_OS_DEF(AF_INET6))
      {
        using namespace std; // For memcpy.
        typename InternetProtocol::endpoint endpoint;
        endpoint.resize(static_cast<std::size_t>(address_info->ai_addrlen));
        memcpy(endpoint.data(), address_info->ai_addr,
            address_info->ai_addrlen);
        results.values_->push_back(
            basic_resolver_entry<InternetProtocol>(endpoint,
              actual_host_name, service_name));
      }
      address_info = address_info->ai_next;
    }

    return results;
  }

  // Create results from an endpoint, host name and service name.
  static basic_resolver_results create(const endpoint_type& endpoint,
      const std::string& host_name, const std::string& service_name)
  {
    basic_resolver_results results;
    results.values_.reset(new values_type);
    results.values_->push_back(
        basic_resolver_entry<InternetProtocol>(
          endpoint, host_name, service_name));
    return results;
  }

  // Create results from a sequence of endpoints, host and service name.
  template <typename EndpointIterator>
  static basic_resolver_results create(
      EndpointIterator begin, EndpointIterator end,
      const std::string& host_name, const std::string& service_name)
  {
    basic_resolver_results results;
    if (begin != end)
    {
      results.values_.reset(new values_type);
      for (EndpointIterator ep_iter = begin; ep_iter != end; ++ep_iter)
      {
        results.values_->push_back(
            basic_resolver_entry<InternetProtocol>(
              *ep_iter, host_name, service_name));
      }
    }
    return results;
  }

# if defined(BOOST_ASIO_WINDOWS_RUNTIME)
  // Create results from a Windows Runtime list of EndpointPair objects.
  static basic_resolver_results create(
      Windows::Foundation::Collections::IVectorView<
        Windows::Networking::EndpointPair^>^ endpoints,
      const boost::asio::detail::addrinfo_type& hints,
      const std::string& host_name, const std::string& service_name)
  {
    basic_resolver_results results;
    if (endpoints->Size)
    {
      results.values_.reset(new values_type);
      for (unsigned int i = 0; i < endpoints->Size; ++i)
      {
        auto pair = endpoints->GetAt(i);

        if (hints.ai_family == BOOST_ASIO_OS_DEF(AF_INET)
            && pair->RemoteHostName->Type
              != Windows::Networking::HostNameType::Ipv4)
          continue;

        if (hints.ai_family == BOOST_ASIO_OS_DEF(AF_INET6)
            && pair->RemoteHostName->Type
              != Windows::Networking::HostNameType::Ipv6)
          continue;

        results.values_->push_back(
            basic_resolver_entry<InternetProtocol>(
              typename InternetProtocol::endpoint(
                ip::make_address(
                  boost::asio::detail::winrt_utils::string(
                    pair->RemoteHostName->CanonicalName)),
                boost::asio::detail::winrt_utils::integer(
                  pair->RemoteServiceName)),
              host_name, service_name));
      }
    }
    return results;
  }
# endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)
#endif // !defined(GENERATING_DOCUMENTATION)

  /// Get the number of entries in the results range.
  size_type size() const BOOST_ASIO_NOEXCEPT
  {
    return this->values_ ? this->values_->size() : 0;
  }

  /// Get the maximum number of entries permitted in a results range.
  size_type max_size() const BOOST_ASIO_NOEXCEPT
  {
    return this->values_ ? this->values_->max_size() : values_type().max_size();
  }

  /// Determine whether the results range is empty.
  bool empty() const BOOST_ASIO_NOEXCEPT
  {
    return this->values_ ? this->values_->empty() : true;
  }

  /// Obtain a begin iterator for the results range.
  const_iterator begin() const
  {
    basic_resolver_results tmp(*this);
    tmp.index_ = 0;
    return BOOST_ASIO_MOVE_CAST(basic_resolver_results)(tmp);
  }

  /// Obtain an end iterator for the results range.
  const_iterator end() const
  {
    return const_iterator();
  }

  /// Obtain a begin iterator for the results range.
  const_iterator cbegin() const
  {
    return begin();
  }

  /// Obtain an end iterator for the results range.
  const_iterator cend() const
  {
    return end();
  }

  /// Swap the results range with another.
  void swap(basic_resolver_results& that) BOOST_ASIO_NOEXCEPT
  {
    if (this != &that)
    {
      this->values_.swap(that.values_);
      std::size_t index = this->index_;
      this->index_ = that.index_;
      that.index_ = index;
    }
  }

  /// Test two iterators for equality.
  friend bool operator==(const basic_resolver_results& a,
      const basic_resolver_results& b)
  {
    return a.equal(b);
  }

  /// Test two iterators for inequality.
  friend bool operator!=(const basic_resolver_results& a,
      const basic_resolver_results& b)
  {
    return !a.equal(b);
  }

private:
  typedef std::vector<basic_resolver_entry<InternetProtocol> > values_type;
};

} // namespace ip
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IP_BASIC_RESOLVER_RESULTS_HPP
