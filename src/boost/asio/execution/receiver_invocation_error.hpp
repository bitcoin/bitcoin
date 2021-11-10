//
// execution/receiver_invocation_error.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP
#define BOOST_ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <stdexcept>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {

/// Exception reported via @c set_error when an exception escapes from
/// @c set_value.
class receiver_invocation_error
  : public std::runtime_error
#if defined(BOOST_ASIO_HAS_STD_NESTED_EXCEPTION)
  , public std::nested_exception
#endif // defined(BOOST_ASIO_HAS_STD_NESTED_EXCEPTION)
{
public:
  /// Constructor.
  BOOST_ASIO_DECL receiver_invocation_error();
};

} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/execution/impl/receiver_invocation_error.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_EXECUTION_RECEIVER_INVOCATION_ERROR_HPP
