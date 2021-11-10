//
// impl/multiple_exceptions.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP
#define BOOST_ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/multiple_exceptions.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)

multiple_exceptions::multiple_exceptions(
    std::exception_ptr first) BOOST_ASIO_NOEXCEPT
  : first_(BOOST_ASIO_MOVE_CAST(std::exception_ptr)(first))
{
}

const char* multiple_exceptions::what() const BOOST_ASIO_NOEXCEPT_OR_NOTHROW
{
  return "multiple exceptions";
}

std::exception_ptr multiple_exceptions::first_exception() const
{
  return first_;
}

#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_IMPL_MULTIPLE_EXCEPTIONS_IPP
