//
// use_awaitable.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_USE_AWAITABLE_HPP
#define BOOST_ASIO_USE_AWAITABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#include <boost/asio/awaitable.hpp>
#include <boost/asio/detail/handler_tracking.hpp>

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# if defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
#  include <boost/asio/detail/source_location.hpp>
# endif // defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

/// A completion token that represents the currently executing coroutine.
/**
 * The @c use_awaitable_t class, with its value @c use_awaitable, is used to
 * represent the currently executing coroutine. This completion token may be
 * passed as a handler to an asynchronous operation. For example:
 *
 * @code awaitable<void> my_coroutine()
 * {
 *   std::size_t n = co_await my_socket.async_read_some(buffer, use_awaitable);
 *   ...
 * } @endcode
 *
 * When used with co_await, the initiating function (@c async_read_some in the
 * above example) suspends the current coroutine. The coroutine is resumed when
 * the asynchronous operation completes, and the result of the operation is
 * returned.
 */
template <typename Executor = any_io_executor>
struct use_awaitable_t
{
  /// Default constructor.
  BOOST_ASIO_CONSTEXPR use_awaitable_t(
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# if defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
      detail::source_location location = detail::source_location::current()
# endif // defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
    )
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# if defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
    : file_name_(location.file_name()),
      line_(location.line()),
      function_name_(location.function_name())
# else // defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
    : file_name_(0),
      line_(0),
      function_name_(0)
# endif // defined(BOOST_ASIO_HAS_SOURCE_LOCATION)
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  {
  }

  /// Constructor used to specify file name, line, and function name.
  BOOST_ASIO_CONSTEXPR use_awaitable_t(const char* file_name,
      int line, const char* function_name)
#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
    : file_name_(file_name),
      line_(line),
      function_name_(function_name)
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  {
#if !defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
    (void)file_name;
    (void)line;
    (void)function_name;
#endif // !defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  }

  /// Adapts an executor to add the @c use_awaitable_t completion token as the
  /// default.
  template <typename InnerExecutor>
  struct executor_with_default : InnerExecutor
  {
    /// Specify @c use_awaitable_t as the default completion token type.
    typedef use_awaitable_t default_completion_token_type;

    /// Construct the adapted executor from the inner executor type.
    template <typename InnerExecutor1>
    executor_with_default(const InnerExecutor1& ex,
        typename constraint<
          conditional<
            !is_same<InnerExecutor1, executor_with_default>::value,
            is_convertible<InnerExecutor1, InnerExecutor>,
            false_type
          >::type::value
        >::type = 0) BOOST_ASIO_NOEXCEPT
      : InnerExecutor(ex)
    {
    }
  };

  /// Type alias to adapt an I/O object to use @c use_awaitable_t as its
  /// default completion token type.
#if defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES) \
  || defined(GENERATING_DOCUMENTATION)
  template <typename T>
  using as_default_on_t = typename T::template rebind_executor<
      executor_with_default<typename T::executor_type> >::other;
#endif // defined(BOOST_ASIO_HAS_ALIAS_TEMPLATES)
       //   || defined(GENERATING_DOCUMENTATION)

  /// Function helper to adapt an I/O object to use @c use_awaitable_t as its
  /// default completion token type.
  template <typename T>
  static typename decay<T>::type::template rebind_executor<
      executor_with_default<typename decay<T>::type::executor_type>
    >::other
  as_default_on(BOOST_ASIO_MOVE_ARG(T) object)
  {
    return typename decay<T>::type::template rebind_executor<
        executor_with_default<typename decay<T>::type::executor_type>
      >::other(BOOST_ASIO_MOVE_CAST(T)(object));
  }

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
  const char* file_name_;
  int line_;
  const char* function_name_;
#endif // defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
};

/// A completion token object that represents the currently executing coroutine.
/**
 * See the documentation for boost::asio::use_awaitable_t for a usage example.
 */
#if defined(GENERATING_DOCUMENTATION)
constexpr use_awaitable_t<> use_awaitable;
#elif defined(BOOST_ASIO_HAS_CONSTEXPR)
constexpr use_awaitable_t<> use_awaitable(0, 0, 0);
#elif defined(BOOST_ASIO_MSVC)
__declspec(selectany) use_awaitable_t<> use_awaitable(0, 0, 0);
#endif

} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include <boost/asio/impl/use_awaitable.hpp>

#endif // defined(BOOST_ASIO_HAS_CO_AWAIT) || defined(GENERATING_DOCUMENTATION)

#endif // BOOST_ASIO_USE_AWAITABLE_HPP
