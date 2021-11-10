//
// execution/detail/as_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP
#define BOOST_ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/detail/as_invocable.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/set_error.hpp>
#include <boost/asio/traits/start_member.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {
namespace detail {

template <typename Executor, typename Receiver>
struct as_operation
{
  typename remove_cvref<Executor>::type ex_;
  typename remove_cvref<Receiver>::type receiver_;
#if !defined(BOOST_ASIO_HAS_MOVE)
  boost::asio::detail::shared_ptr<boost::asio::detail::atomic_count> ref_count_;
#endif // !defined(BOOST_ASIO_HAS_MOVE)

  template <typename E, typename R>
  explicit as_operation(BOOST_ASIO_MOVE_ARG(E) e, BOOST_ASIO_MOVE_ARG(R) r)
    : ex_(BOOST_ASIO_MOVE_CAST(E)(e)),
      receiver_(BOOST_ASIO_MOVE_CAST(R)(r))
#if !defined(BOOST_ASIO_HAS_MOVE)
      , ref_count_(new boost::asio::detail::atomic_count(1))
#endif // !defined(BOOST_ASIO_HAS_MOVE)
  {
  }

  void start() BOOST_ASIO_NOEXCEPT
  {
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
      execution::execute(
          BOOST_ASIO_MOVE_CAST(typename remove_cvref<Executor>::type)(ex_),
          as_invocable<typename remove_cvref<Receiver>::type,
              Executor>(receiver_
#if !defined(BOOST_ASIO_HAS_MOVE)
                , ref_count_
#endif // !defined(BOOST_ASIO_HAS_MOVE)
              ));
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(
          BOOST_ASIO_MOVE_OR_LVALUE(
            typename remove_cvref<Receiver>::type)(
              receiver_),
          std::current_exception());
#else // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
};

} // namespace detail
} // namespace execution
namespace traits {

#if !defined(BOOST_ASIO_HAS_DEDUCED_START_MEMBER_TRAIT)

template <typename Executor, typename Receiver>
struct start_member<
    boost::asio::execution::detail::as_operation<Executor, Receiver> >
{
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_valid = true);
  BOOST_ASIO_STATIC_CONSTEXPR(bool, is_noexcept = true);
  typedef void result_type;
};

#endif // !defined(BOOST_ASIO_HAS_DEDUCED_START_MEMBER_TRAIT)

} // namespace traits
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_DETAIL_AS_OPERATION_HPP
