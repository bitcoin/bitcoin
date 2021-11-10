//
// multiple_exceptions.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_MULTIPLE_EXCEPTIONS_HPP
#define BOOST_ASIO_MULTIPLE_EXCEPTIONS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <exception>
#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR) \
  || defined(GENERATING_DOCUMENTATION)

/// Exception thrown when there are multiple pending exceptions to rethrow.
class multiple_exceptions
  : public std::exception
{
public:
  /// Constructor.
  BOOST_ASIO_DECL multiple_exceptions(
      std::exception_ptr first) BOOST_ASIO_NOEXCEPT;

  /// Obtain message associated with exception.
  BOOST_ASIO_DECL virtual const char* what() const
    BOOST_ASIO_NOEXCEPT_OR_NOTHROW;

  /// Obtain a pointer to the first exception.
  BOOST_ASIO_DECL std::exception_ptr first_exception() const;

private:
  std::exception_ptr first_;
};

#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
       //   || defined(GENERATING_DOCUMENTATION)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/impl/multiple_exceptions.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_MULTIPLE_EXCEPTIONS_HPP
