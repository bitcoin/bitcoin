//
// detail/handler_work.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_HANDLER_WORK_HPP
#define BOOST_ASIO_DETAIL_HANDLER_WORK_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/execution/allocator.hpp>
#include <boost/asio/execution/blocking.hpp>
#include <boost/asio/execution/execute.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/outstanding_work.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/prefer.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {

class executor;
class io_context;

#if !defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

class any_io_executor;

#endif // !defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

namespace execution {

#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename...> class any_executor;

#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

template <typename, typename, typename, typename, typename,
    typename, typename, typename, typename> class any_executor;

#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)

} // namespace execution
namespace detail {

template <typename Executor, typename CandidateExecutor = void,
    typename IoContext = io_context,
    typename PolymorphicExecutor = executor, typename = void>
class handler_work_base
{
public:
  explicit handler_work_base(int, int, const Executor& ex) BOOST_ASIO_NOEXCEPT
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(bool /*base1_owns_work*/, const Executor& ex,
      const OtherExecutor& /*candidate*/) BOOST_ASIO_NOEXCEPT
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  handler_work_base(const handler_work_base& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  handler_work_base(handler_work_base&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(executor_type)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return true;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    execution::execute(
        boost::asio::prefer(executor_,
          execution::blocking.possibly,
          execution::allocator((get_associated_allocator)(handler))),
        BOOST_ASIO_MOVE_CAST(Function)(function));
  }

private:
  typedef typename decay<
      typename prefer_result<Executor,
        execution::outstanding_work_t::tracked_t
      >::type
    >::type executor_type;

  executor_type executor_;
};

template <typename Executor, typename CandidateExecutor,
    typename IoContext, typename PolymorphicExecutor>
class handler_work_base<Executor, CandidateExecutor,
    IoContext, PolymorphicExecutor,
    typename enable_if<
      !execution::is_executor<Executor>::value
        && (!is_same<Executor, PolymorphicExecutor>::value
          || !is_same<CandidateExecutor, void>::value)
    >::type>
{
public:
  explicit handler_work_base(int, int, const Executor& ex) BOOST_ASIO_NOEXCEPT
    : executor_(ex),
      owns_work_(true)
  {
    executor_.on_work_started();
  }

  handler_work_base(bool /*base1_owns_work*/, const Executor& ex,
      const Executor& candidate) BOOST_ASIO_NOEXCEPT
    : executor_(ex),
      owns_work_(ex != candidate)
  {
    if (owns_work_)
      executor_.on_work_started();
  }

  template <typename OtherExecutor>
  handler_work_base(bool /*base1_owns_work*/, const Executor& ex,
      const OtherExecutor& /*candidate*/) BOOST_ASIO_NOEXCEPT
    : executor_(ex),
      owns_work_(true)
  {
    executor_.on_work_started();
  }

  handler_work_base(const handler_work_base& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_),
      owns_work_(other.owns_work_)
  {
    if (owns_work_)
      executor_.on_work_started();
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  handler_work_base(handler_work_base&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_)),
      owns_work_(other.owns_work_)
  {
    other.owns_work_ = false;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  ~handler_work_base()
  {
    if (owns_work_)
      executor_.on_work_finished();
  }

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return owns_work_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    executor_.dispatch(BOOST_ASIO_MOVE_CAST(Function)(function),
        boost::asio::get_associated_allocator(handler));
  }

private:
  Executor executor_;
  bool owns_work_;
};

template <typename Executor, typename IoContext, typename PolymorphicExecutor>
class handler_work_base<Executor, void, IoContext, PolymorphicExecutor,
    typename enable_if<
      is_same<
        Executor,
        typename IoContext::executor_type
      >::value
    >::type>
{
public:
  explicit handler_work_base(int, int, const Executor&)
  {
  }

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return false;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    // When using a native implementation, I/O completion handlers are
    // already dispatched according to the execution context's executor's
    // rules. We can call the function directly.
    boost_asio_handler_invoke_helpers::invoke(function, handler);
  }
};

template <typename Executor, typename IoContext>
class handler_work_base<Executor, void, IoContext, Executor>
{
public:
  explicit handler_work_base(int, int, const Executor& ex) BOOST_ASIO_NOEXCEPT
#if !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(
        ex.target_type() == typeid(typename IoContext::executor_type)
          ? Executor() : ex)
#else // !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(ex)
#endif // !defined(BOOST_ASIO_NO_TYPEID)
  {
    if (executor_)
      executor_.on_work_started();
  }

  handler_work_base(bool /*base1_owns_work*/, const Executor& ex,
      const Executor& candidate) BOOST_ASIO_NOEXCEPT
    : executor_(ex != candidate ? ex : Executor())
  {
    if (executor_)
      executor_.on_work_started();
  }

  template <typename OtherExecutor>
  handler_work_base(const Executor& ex,
      const OtherExecutor&) BOOST_ASIO_NOEXCEPT
    : executor_(ex)
  {
    executor_.on_work_started();
  }

  handler_work_base(const handler_work_base& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
    if (executor_)
      executor_.on_work_started();
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  handler_work_base(handler_work_base&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(Executor)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  ~handler_work_base()
  {
    if (executor_)
      executor_.on_work_finished();
  }

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler& handler)
  {
    executor_.dispatch(BOOST_ASIO_MOVE_CAST(Function)(function),
        boost::asio::get_associated_allocator(handler));
  }

private:
  Executor executor_;
};

template <
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    typename... SupportableProperties,
#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    typename T1, typename T2, typename T3, typename T4, typename T5,
    typename T6, typename T7, typename T8, typename T9,
#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    typename CandidateExecutor, typename IoContext,
    typename PolymorphicExecutor>
class handler_work_base<
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    execution::any_executor<SupportableProperties...>,
#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    execution::any_executor<T1, T2, T3, T4, T5, T6, T7, T8, T9>,
#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    CandidateExecutor, IoContext, PolymorphicExecutor>
{
public:
  typedef
#if defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    execution::any_executor<SupportableProperties...>
#else // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    execution::any_executor<T1, T2, T3, T4, T5, T6, T7, T8, T9>
#endif // defined(BOOST_ASIO_HAS_VARIADIC_TEMPLATES)
    executor_type;

  explicit handler_work_base(int, int,
      const executor_type& ex) BOOST_ASIO_NOEXCEPT
#if !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(
        ex.target_type() == typeid(typename IoContext::executor_type)
          ? executor_type()
          : boost::asio::prefer(ex, execution::outstanding_work.tracked))
#else // !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
#endif // !defined(BOOST_ASIO_NO_TYPEID)
  {
  }

  handler_work_base(bool base1_owns_work, const executor_type& ex,
      const executor_type& candidate) BOOST_ASIO_NOEXCEPT
    : executor_(
        !base1_owns_work && ex == candidate
          ? executor_type()
          : boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(bool /*base1_owns_work*/, const executor_type& ex,
      const OtherExecutor& /*candidate*/) BOOST_ASIO_NOEXCEPT
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  handler_work_base(const handler_work_base& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  handler_work_base(handler_work_base&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(executor_type)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler&)
  {
    execution::execute(
        boost::asio::prefer(executor_, execution::blocking.possibly),
        BOOST_ASIO_MOVE_CAST(Function)(function));
  }

private:
  executor_type executor_;
};

#if !defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Executor, typename CandidateExecutor,
    typename IoContext, typename PolymorphicExecutor>
class handler_work_base<
    Executor, CandidateExecutor,
    IoContext, PolymorphicExecutor,
    typename enable_if<
      is_same<
        Executor,
        any_io_executor
      >::value
    >::type>
{
public:
  typedef Executor executor_type;

  explicit handler_work_base(int, int,
      const executor_type& ex) BOOST_ASIO_NOEXCEPT
#if !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(
        ex.target_type() == typeid(typename IoContext::executor_type)
          ? executor_type()
          : boost::asio::prefer(ex, execution::outstanding_work.tracked))
#else // !defined(BOOST_ASIO_NO_TYPEID)
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
#endif // !defined(BOOST_ASIO_NO_TYPEID)
  {
  }

  handler_work_base(bool base1_owns_work, const executor_type& ex,
      const executor_type& candidate) BOOST_ASIO_NOEXCEPT
    : executor_(
        !base1_owns_work && ex == candidate
          ? executor_type()
          : boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  template <typename OtherExecutor>
  handler_work_base(bool /*base1_owns_work*/, const executor_type& ex,
      const OtherExecutor& /*candidate*/) BOOST_ASIO_NOEXCEPT
    : executor_(boost::asio::prefer(ex, execution::outstanding_work.tracked))
  {
  }

  handler_work_base(const handler_work_base& other) BOOST_ASIO_NOEXCEPT
    : executor_(other.executor_)
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  handler_work_base(handler_work_base&& other) BOOST_ASIO_NOEXCEPT
    : executor_(BOOST_ASIO_MOVE_CAST(executor_type)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  bool owns_work() const BOOST_ASIO_NOEXCEPT
  {
    return !!executor_;
  }

  template <typename Function, typename Handler>
  void dispatch(Function& function, Handler&)
  {
    execution::execute(
        boost::asio::prefer(executor_, execution::blocking.possibly),
        BOOST_ASIO_MOVE_CAST(Function)(function));
  }

private:
  executor_type executor_;
};

#endif // !defined(BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT)

template <typename Handler, typename IoExecutor, typename = void>
class handler_work :
  handler_work_base<IoExecutor>,
  handler_work_base<typename associated_executor<
      Handler, IoExecutor>::type, IoExecutor>
{
public:
  typedef handler_work_base<IoExecutor> base1_type;
  typedef handler_work_base<typename associated_executor<
    Handler, IoExecutor>::type, IoExecutor> base2_type;

  handler_work(Handler& handler, const IoExecutor& io_ex) BOOST_ASIO_NOEXCEPT
    : base1_type(0, 0, io_ex),
      base2_type(base1_type::owns_work(),
          boost::asio::get_associated_executor(handler, io_ex), io_ex)
  {
  }

  template <typename Function>
  void complete(Function& function, Handler& handler)
  {
    if (!base1_type::owns_work() && !base2_type::owns_work())
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
      boost_asio_handler_invoke_helpers::invoke(function, handler);
    }
    else
    {
      base2_type::dispatch(function, handler);
    }
  }
};

template <typename Handler, typename IoExecutor>
class handler_work<
    Handler, IoExecutor,
    typename enable_if<
      is_same<
        typename associated_executor<Handler,
          IoExecutor>::asio_associated_executor_is_unspecialised,
        void
      >::value
    >::type> : handler_work_base<IoExecutor>
{
public:
  typedef handler_work_base<IoExecutor> base1_type;

  handler_work(Handler&, const IoExecutor& io_ex) BOOST_ASIO_NOEXCEPT
    : base1_type(0, 0, io_ex)
  {
  }

  template <typename Function>
  void complete(Function& function, Handler& handler)
  {
    if (!base1_type::owns_work())
    {
      // When using a native implementation, I/O completion handlers are
      // already dispatched according to the execution context's executor's
      // rules. We can call the function directly.
      boost_asio_handler_invoke_helpers::invoke(function, handler);
    }
    else
    {
      base1_type::dispatch(function, handler);
    }
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_HANDLER_WORK_HPP
