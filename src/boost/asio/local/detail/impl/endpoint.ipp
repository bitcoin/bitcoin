//
// local/detail/impl/endpoint.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Derived from a public domain implementation written by Daniel Casimiro.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_LOCAL_DETAIL_IMPL_ENDPOINT_IPP
#define BOOST_ASIO_LOCAL_DETAIL_IMPL_ENDPOINT_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#include <cstring>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/local/detail/endpoint.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace local {
namespace detail {

endpoint::endpoint()
{
  init("", 0);
}

endpoint::endpoint(const char* path_name)
{
  using namespace std; // For strlen.
  init(path_name, strlen(path_name));
}

endpoint::endpoint(const std::string& path_name)
{
  init(path_name.data(), path_name.length());
}

#if defined(BOOST_ASIO_HAS_STRING_VIEW)
endpoint::endpoint(string_view path_name)
{
  init(path_name.data(), path_name.length());
}
#endif // defined(BOOST_ASIO_HAS_STRING_VIEW)

void endpoint::resize(std::size_t new_size)
{
  if (new_size > sizeof(boost::asio::detail::sockaddr_un_type))
  {
    boost::system::error_code ec(boost::asio::error::invalid_argument);
    boost::asio::detail::throw_error(ec);
  }
  else if (new_size == 0)
  {
    path_length_ = 0;
  }
  else
  {
    path_length_ = new_size
      - offsetof(boost::asio::detail::sockaddr_un_type, sun_path);

    // The path returned by the operating system may be NUL-terminated.
    if (path_length_ > 0 && data_.local.sun_path[path_length_ - 1] == 0)
      --path_length_;
  }
}

std::string endpoint::path() const
{
  return std::string(data_.local.sun_path, path_length_);
}

void endpoint::path(const char* p)
{
  using namespace std; // For strlen.
  init(p, strlen(p));
}

void endpoint::path(const std::string& p)
{
  init(p.data(), p.length());
}

bool operator==(const endpoint& e1, const endpoint& e2)
{
  return e1.path() == e2.path();
}

bool operator<(const endpoint& e1, const endpoint& e2)
{
  return e1.path() < e2.path();
}

void endpoint::init(const char* path_name, std::size_t path_length)
{
  if (path_length > sizeof(data_.local.sun_path) - 1)
  {
    // The buffer is not large enough to store this address.
    boost::system::error_code ec(boost::asio::error::name_too_long);
    boost::asio::detail::throw_error(ec);
  }

  using namespace std; // For memset and memcpy.
  memset(&data_.local, 0, sizeof(boost::asio::detail::sockaddr_un_type));
  data_.local.sun_family = AF_UNIX;
  if (path_length > 0)
    memcpy(data_.local.sun_path, path_name, path_length);
  path_length_ = path_length;
}

} // namespace detail
} // namespace local
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif // BOOST_ASIO_LOCAL_DETAIL_IMPL_ENDPOINT_IPP
