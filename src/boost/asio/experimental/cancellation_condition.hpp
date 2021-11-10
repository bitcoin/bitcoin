//
// experimental/cancellation_condition.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_CANCELLATION_CONDITION_HPP
#define BOOST_ASIO_EXPERIMENTAL_CANCELLATION_CONDITION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <exception>
#include <boost/asio/cancellation_type.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/detail/type_traits.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {

/// Wait for all operations to complete.
class wait_for_all
{
public:
  template <typename... Args>
  BOOST_ASIO_CONSTEXPR cancellation_type_t operator()(
      Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_type::none;
  }
};

/// Wait until an operation completes, then cancel the others.
class wait_for_one
{
public:
  BOOST_ASIO_CONSTEXPR explicit wait_for_one(
      cancellation_type_t cancel_type = cancellation_type::all)
    : cancel_type_(cancel_type)
  {
  }

  template <typename... Args>
  BOOST_ASIO_CONSTEXPR cancellation_type_t operator()(
      Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return cancel_type_;
  }

private:
  cancellation_type_t cancel_type_;
};

/// Wait until an operation completes without an error, then cancel the others.
/**
 * If no operation completes without an error, waits for completion of all
 * operations.
 */
class wait_for_one_success
{
public:
  BOOST_ASIO_CONSTEXPR explicit wait_for_one_success(
      cancellation_type_t cancel_type = cancellation_type::all)
    : cancel_type_(cancel_type)
  {
  }

  BOOST_ASIO_CONSTEXPR cancellation_type_t
  operator()() const BOOST_ASIO_NOEXCEPT
  {
    return cancel_type_;
  }

  template <typename E, typename... Args>
  BOOST_ASIO_CONSTEXPR typename constraint<
    !is_same<typename decay<E>::type, boost::system::error_code>::value
      && !is_same<typename decay<E>::type, std::exception_ptr>::value,
    cancellation_type_t
  >::type operator()(const E&, Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return cancel_type_;
  }

  template <typename E, typename... Args>
  BOOST_ASIO_CONSTEXPR typename constraint<
      is_same<typename decay<E>::type, boost::system::error_code>::value
        || is_same<typename decay<E>::type, std::exception_ptr>::value,
      cancellation_type_t
  >::type operator()(const E& e, Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return !!e ? cancellation_type::none : cancel_type_;
  }

private:
  cancellation_type_t cancel_type_;
};

/// Wait until an operation completes with an error, then cancel the others.
/**
 * If no operation completes with an error, waits for completion of all
 * operations.
 */
class wait_for_one_error
{
public:
  BOOST_ASIO_CONSTEXPR explicit wait_for_one_error(
      cancellation_type_t cancel_type = cancellation_type::all)
    : cancel_type_(cancel_type)
  {
  }

  BOOST_ASIO_CONSTEXPR cancellation_type_t
  operator()() const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_type::none;
  }

  template <typename E, typename... Args>
  BOOST_ASIO_CONSTEXPR typename constraint<
    !is_same<typename decay<E>::type, boost::system::error_code>::value
      && !is_same<typename decay<E>::type, std::exception_ptr>::value,
    cancellation_type_t
  >::type operator()(const E&, Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return cancellation_type::none;
  }

  template <typename E, typename... Args>
  BOOST_ASIO_CONSTEXPR typename constraint<
      is_same<typename decay<E>::type, boost::system::error_code>::value
        || is_same<typename decay<E>::type, std::exception_ptr>::value,
      cancellation_type_t
  >::type operator()(const E& e, Args&&...) const BOOST_ASIO_NOEXCEPT
  {
    return !!e ? cancel_type_ : cancellation_type::none;
  }

private:
  cancellation_type_t cancel_type_;
};

} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_CANCELLATION_CONDITION_HPP
