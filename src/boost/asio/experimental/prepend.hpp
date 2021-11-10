//
// experimental/prepend.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_PREPEND_HPP
#define BOOST_ASIO_EXPERIMENTAL_PREPEND_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <tuple>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {

/// Completion token type used to specify that the completion handler
/// arguments should be passed additional values before the results of the
/// operation.
template <typename CompletionToken, typename... Values>
class prepend_t
{
public:
  /// Constructor.
  template <typename T, typename... V>
  BOOST_ASIO_CONSTEXPR explicit prepend_t(
      BOOST_ASIO_MOVE_ARG(T) completion_token,
      BOOST_ASIO_MOVE_ARG(V)... values)
    : token_(BOOST_ASIO_MOVE_CAST(T)(completion_token)),
      values_(BOOST_ASIO_MOVE_CAST(V)(values)...)
  {
  }

//private:
  CompletionToken token_;
  std::tuple<Values...> values_;
};

/// Completion token type used to specify that the completion handler
/// arguments should be passed additional values before the results of the
/// operation.
template <typename CompletionToken, typename... Values>
inline BOOST_ASIO_CONSTEXPR prepend_t<
  typename decay<CompletionToken>::type, typename decay<Values>::type...>
prepend(BOOST_ASIO_MOVE_ARG(CompletionToken) completion_token,
    BOOST_ASIO_MOVE_ARG(Values)... values)
{
  return prepend_t<
    typename decay<CompletionToken>::type, typename decay<Values>::type...>(
      BOOST_ASIO_MOVE_CAST(CompletionToken)(completion_token),
      BOOST_ASIO_MOVE_CAST(Values)(values)...);
}

} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/experimental/impl/prepend.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_PREPEND_HPP
