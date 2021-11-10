//
// exection/impl/bad_executor.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
#define BOOST_ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/execution/bad_executor.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {

bad_executor::bad_executor() BOOST_ASIO_NOEXCEPT
{
}

const char* bad_executor::what() const BOOST_ASIO_NOEXCEPT_OR_NOTHROW
{
  return "bad executor";
}

} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_IMPL_BAD_EXECUTOR_IPP
