//
// execution/detail/as_invocable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP
#define BOOST_ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/atomic_count.hpp>
#include <boost/asio/detail/memory.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/receiver_invocation_error.hpp>
#include <boost/asio/execution/set_done.hpp>
#include <boost/asio/execution/set_error.hpp>
#include <boost/asio/execution/set_value.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace execution {
namespace detail {

#if defined(BOOST_ASIO_HAS_MOVE)

template <typename Receiver, typename>
struct as_invocable
{
  Receiver* receiver_;

  explicit as_invocable(Receiver& r) BOOST_ASIO_NOEXCEPT
    : receiver_(boost::asio::detail::addressof(r))
  {
  }

  as_invocable(as_invocable&& other) BOOST_ASIO_NOEXCEPT
    : receiver_(other.receiver_)
  {
    other.receiver_ = 0;
  }

  ~as_invocable()
  {
    if (receiver_)
      execution::set_done(BOOST_ASIO_MOVE_OR_LVALUE(Receiver)(*receiver_));
  }

  void operator()() BOOST_ASIO_LVALUE_REF_QUAL BOOST_ASIO_NOEXCEPT
  {
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
      execution::set_value(BOOST_ASIO_MOVE_CAST(Receiver)(*receiver_));
      receiver_ = 0;
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    }
    catch (...)
    {
#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(BOOST_ASIO_MOVE_CAST(Receiver)(*receiver_),
          std::make_exception_ptr(receiver_invocation_error()));
      receiver_ = 0;
#else // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
};

#else // defined(BOOST_ASIO_HAS_MOVE)

template <typename Receiver, typename>
struct as_invocable
{
  Receiver* receiver_;
  boost::asio::detail::shared_ptr<boost::asio::detail::atomic_count> ref_count_;

  explicit as_invocable(Receiver& r,
      const boost::asio::detail::shared_ptr<
        boost::asio::detail::atomic_count>& c) BOOST_ASIO_NOEXCEPT
    : receiver_(boost::asio::detail::addressof(r)),
      ref_count_(c)
  {
  }

  as_invocable(const as_invocable& other) BOOST_ASIO_NOEXCEPT
    : receiver_(other.receiver_),
      ref_count_(other.ref_count_)
  {
    ++(*ref_count_);
  }

  ~as_invocable()
  {
    if (--(*ref_count_) == 0)
      execution::set_done(*receiver_);
  }

  void operator()() BOOST_ASIO_LVALUE_REF_QUAL BOOST_ASIO_NOEXCEPT
  {
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    try
    {
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
      execution::set_value(*receiver_);
      ++(*ref_count_);
    }
#if !defined(BOOST_ASIO_NO_EXCEPTIONS)
    catch (...)
    {
#if defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      execution::set_error(*receiver_,
          std::make_exception_ptr(receiver_invocation_error()));
      ++(*ref_count_);
#else // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
      std::terminate();
#endif // defined(BOOST_ASIO_HAS_STD_EXCEPTION_PTR)
    }
#endif // !defined(BOOST_ASIO_NO_EXCEPTIONS)
  }
};

#endif // defined(BOOST_ASIO_HAS_MOVE)

template <typename T>
struct is_as_invocable : false_type
{
};

template <typename Function, typename T>
struct is_as_invocable<as_invocable<Function, T> > : true_type
{
};

} // namespace detail
} // namespace execution
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXECUTION_DETAIL_AS_INVOCABLE_HPP
