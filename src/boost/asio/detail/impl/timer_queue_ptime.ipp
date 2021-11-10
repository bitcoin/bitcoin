//
// detail/impl/timer_queue_ptime.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_PTIME_IPP
#define BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_PTIME_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#include <boost/asio/detail/timer_queue_ptime.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

timer_queue<time_traits<boost::posix_time::ptime> >::timer_queue()
{
}

timer_queue<time_traits<boost::posix_time::ptime> >::~timer_queue()
{
}

bool timer_queue<time_traits<boost::posix_time::ptime> >::enqueue_timer(
    const time_type& time, per_timer_data& timer, wait_op* op)
{
  return impl_.enqueue_timer(time, timer, op);
}

bool timer_queue<time_traits<boost::posix_time::ptime> >::empty() const
{
  return impl_.empty();
}

long timer_queue<time_traits<boost::posix_time::ptime> >::wait_duration_msec(
    long max_duration) const
{
  return impl_.wait_duration_msec(max_duration);
}

long timer_queue<time_traits<boost::posix_time::ptime> >::wait_duration_usec(
    long max_duration) const
{
  return impl_.wait_duration_usec(max_duration);
}

void timer_queue<time_traits<boost::posix_time::ptime> >::get_ready_timers(
    op_queue<operation>& ops)
{
  impl_.get_ready_timers(ops);
}

void timer_queue<time_traits<boost::posix_time::ptime> >::get_all_timers(
    op_queue<operation>& ops)
{
  impl_.get_all_timers(ops);
}

std::size_t timer_queue<time_traits<boost::posix_time::ptime> >::cancel_timer(
    per_timer_data& timer, op_queue<operation>& ops, std::size_t max_cancelled)
{
  return impl_.cancel_timer(timer, ops, max_cancelled);
}

void timer_queue<time_traits<boost::posix_time::ptime> >::cancel_timer_by_key(
    per_timer_data* timer, op_queue<operation>& ops, void* cancellation_key)
{
  impl_.cancel_timer_by_key(timer, ops, cancellation_key);
}

void timer_queue<time_traits<boost::posix_time::ptime> >::move_timer(
    per_timer_data& target, per_timer_data& source)
{
  impl_.move_timer(target, source);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#endif // BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_PTIME_IPP
