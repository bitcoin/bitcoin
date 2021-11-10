//
// detail/impl/winrt_timer_scheduler.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_WINRT_TIMER_SCHEDULER_IPP
#define BOOST_ASIO_DETAIL_IMPL_WINRT_TIMER_SCHEDULER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_WINDOWS_RUNTIME)

#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/winrt_timer_scheduler.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

winrt_timer_scheduler::winrt_timer_scheduler(execution_context& context)
  : execution_context_service_base<winrt_timer_scheduler>(context),
    scheduler_(use_service<scheduler_impl>(context)),
    mutex_(),
    event_(),
    timer_queues_(),
    thread_(0),
    stop_thread_(false),
    shutdown_(false)
{
  thread_ = new boost::asio::detail::thread(
      bind_handler(&winrt_timer_scheduler::call_run_thread, this));
}

winrt_timer_scheduler::~winrt_timer_scheduler()
{
  shutdown();
}

void winrt_timer_scheduler::shutdown()
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  shutdown_ = true;
  stop_thread_ = true;
  event_.signal(lock);
  lock.unlock();

  if (thread_)
  {
    thread_->join();
    delete thread_;
    thread_ = 0;
  }

  op_queue<operation> ops;
  timer_queues_.get_all_timers(ops);
  scheduler_.abandon_operations(ops);
}

void winrt_timer_scheduler::notify_fork(execution_context::fork_event)
{
}

void winrt_timer_scheduler::init_task()
{
}

void winrt_timer_scheduler::run_thread()
{
  boost::asio::detail::mutex::scoped_lock lock(mutex_);
  while (!stop_thread_)
  {
    const long max_wait_duration = 5 * 60 * 1000000;
    long wait_duration = timer_queues_.wait_duration_usec(max_wait_duration);
    event_.wait_for_usec(lock, wait_duration);
    event_.clear(lock);
    op_queue<operation> ops;
    timer_queues_.get_ready_timers(ops);
    if (!ops.empty())
    {
      lock.unlock();
      scheduler_.post_deferred_completions(ops);
      lock.lock();
    }
  }
}

void winrt_timer_scheduler::call_run_thread(winrt_timer_scheduler* scheduler)
{
  scheduler->run_thread();
}

void winrt_timer_scheduler::do_add_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.insert(&queue);
}

void winrt_timer_scheduler::do_remove_timer_queue(timer_queue_base& queue)
{
  mutex::scoped_lock lock(mutex_);
  timer_queues_.erase(&queue);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_WINDOWS_RUNTIME)

#endif // BOOST_ASIO_DETAIL_IMPL_WINRT_TIMER_SCHEDULER_IPP
