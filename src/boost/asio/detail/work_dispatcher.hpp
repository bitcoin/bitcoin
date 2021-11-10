//
// detail/work_dispatcher.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_WORK_DISPATCHER_HPP
#define BOOST_ASIO_DETAIL_WORK_DISPATCHER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/associated_allocator.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution/allocator.hpp>
#include <boost/asio/execution/blocking.hpp>
#include <boost/asio/execution/outstanding_work.hpp>
#include <boost/asio/prefer.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

template <typename Handler, typename Executor, typename = void>
struct is_work_dispatcher_required : true_type
{
};

template <typename Handler, typename Executor>
struct is_work_dispatcher_required<Handler, Executor,
    typename enable_if<
      is_same<
        typename associated_executor<Handler,
          Executor>::asio_associated_executor_is_unspecialised,
        void
      >::value
    >::type> : false_type
{
};

template <typename Handler, typename Executor, typename = void>
class work_dispatcher
{
public:
  template <typename CompletionHandler>
  work_dispatcher(BOOST_ASIO_MOVE_ARG(CompletionHandler) handler,
      const Executor& handler_ex)
    : handler_(BOOST_ASIO_MOVE_CAST(CompletionHandler)(handler)),
      executor_(boost::asio::prefer(handler_ex,
          execution::outstanding_work.tracked))
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  work_dispatcher(const work_dispatcher& other)
    : handler_(other.handler_),
      executor_(other.executor_)
  {
  }

  work_dispatcher(work_dispatcher&& other)
    : handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_)),
      executor_(BOOST_ASIO_MOVE_CAST(work_executor_type)(other.executor_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    execution::execute(
        boost::asio::prefer(executor_,
          execution::blocking.possibly,
          execution::allocator((get_associated_allocator)(handler_))),
        boost::asio::detail::bind_handler(
          BOOST_ASIO_MOVE_CAST(Handler)(handler_)));
  }

private:
  typedef typename decay<
      typename prefer_result<const Executor&,
        execution::outstanding_work_t::tracked_t
      >::type
    >::type work_executor_type;

  Handler handler_;
  work_executor_type executor_;
};

#if !defined(BOOST_ASIO_NO_TS_EXECUTORS)

template <typename Handler, typename Executor>
class work_dispatcher<Handler, Executor,
    typename enable_if<!execution::is_executor<Executor>::value>::type>
{
public:
  template <typename CompletionHandler>
  work_dispatcher(BOOST_ASIO_MOVE_ARG(CompletionHandler) handler,
      const Executor& handler_ex)
    : work_(handler_ex),
      handler_(BOOST_ASIO_MOVE_CAST(CompletionHandler)(handler))
  {
  }

#if defined(BOOST_ASIO_HAS_MOVE)
  work_dispatcher(const work_dispatcher& other)
    : work_(other.work_),
      handler_(other.handler_)
  {
  }

  work_dispatcher(work_dispatcher&& other)
    : work_(BOOST_ASIO_MOVE_CAST(executor_work_guard<Executor>)(other.work_)),
      handler_(BOOST_ASIO_MOVE_CAST(Handler)(other.handler_))
  {
  }
#endif // defined(BOOST_ASIO_HAS_MOVE)

  void operator()()
  {
    typename associated_allocator<Handler>::type alloc(
        (get_associated_allocator)(handler_));
    work_.get_executor().dispatch(
        boost::asio::detail::bind_handler(
          BOOST_ASIO_MOVE_CAST(Handler)(handler_)), alloc);
    work_.reset();
  }

private:
  executor_work_guard<Executor> work_;
  Handler handler_;
};

#endif // !defined(BOOST_ASIO_NO_TS_EXECUTORS)

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_WORK_DISPATCHER_HPP
