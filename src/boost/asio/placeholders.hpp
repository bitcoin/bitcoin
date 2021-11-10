//
// placeholders.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_PLACEHOLDERS_HPP
#define BOOST_ASIO_PLACEHOLDERS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <boost/bind/arg.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace placeholders {

#if defined(GENERATING_DOCUMENTATION)

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the error argument of a handler for any of the asynchronous functions.
unspecified error;

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the bytes_transferred argument of a handler for asynchronous functions such
/// as boost::asio::basic_stream_socket::async_write_some or
/// boost::asio::async_write.
unspecified bytes_transferred;

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the iterator argument of a handler for asynchronous functions such as
/// boost::asio::async_connect.
unspecified iterator;

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the results argument of a handler for asynchronous functions such as
/// boost::asio::basic_resolver::async_resolve.
unspecified results;

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the results argument of a handler for asynchronous functions such as
/// boost::asio::async_connect.
unspecified endpoint;

/// An argument placeholder, for use with boost::bind(), that corresponds to
/// the signal_number argument of a handler for asynchronous functions such as
/// boost::asio::signal_set::async_wait.
unspecified signal_number;

#elif defined(BOOST_ASIO_HAS_BOOST_BIND)
# if defined(__BORLANDC__) || defined(__GNUC__)

inline boost::arg<1> error()
{
  return boost::arg<1>();
}

inline boost::arg<2> bytes_transferred()
{
  return boost::arg<2>();
}

inline boost::arg<2> iterator()
{
  return boost::arg<2>();
}

inline boost::arg<2> results()
{
  return boost::arg<2>();
}

inline boost::arg<2> endpoint()
{
  return boost::arg<2>();
}

inline boost::arg<2> signal_number()
{
  return boost::arg<2>();
}

# else

namespace detail
{
  template <int Number>
  struct placeholder
  {
    static boost::arg<Number>& get()
    {
      static boost::arg<Number> result;
      return result;
    }
  };
}

#  if defined(BOOST_ASIO_MSVC) && (BOOST_ASIO_MSVC < 1400)

static boost::arg<1>& error
  = boost::asio::placeholders::detail::placeholder<1>::get();
static boost::arg<2>& bytes_transferred
  = boost::asio::placeholders::detail::placeholder<2>::get();
static boost::arg<2>& iterator
  = boost::asio::placeholders::detail::placeholder<2>::get();
static boost::arg<2>& results
  = boost::asio::placeholders::detail::placeholder<2>::get();
static boost::arg<2>& endpoint
  = boost::asio::placeholders::detail::placeholder<2>::get();
static boost::arg<2>& signal_number
  = boost::asio::placeholders::detail::placeholder<2>::get();

#  else

namespace
{
  boost::arg<1>& error
    = boost::asio::placeholders::detail::placeholder<1>::get();
  boost::arg<2>& bytes_transferred
    = boost::asio::placeholders::detail::placeholder<2>::get();
  boost::arg<2>& iterator
    = boost::asio::placeholders::detail::placeholder<2>::get();
  boost::arg<2>& results
    = boost::asio::placeholders::detail::placeholder<2>::get();
  boost::arg<2>& endpoint
    = boost::asio::placeholders::detail::placeholder<2>::get();
  boost::arg<2>& signal_number
    = boost::asio::placeholders::detail::placeholder<2>::get();
} // namespace

#  endif
# endif
#endif

} // namespace placeholders
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_PLACEHOLDERS_HPP
