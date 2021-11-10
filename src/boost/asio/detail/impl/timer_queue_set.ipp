//
// detail/impl/timer_queue_set.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_SET_IPP
#define BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_SET_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/timer_queue_set.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

timer_queue_set::timer_queue_set()
  : first_(0)
{
}

void timer_queue_set::insert(timer_queue_base* q)
{
  q->next_ = first_;
  first_ = q;
}

void timer_queue_set::erase(timer_queue_base* q)
{
  if (first_)
  {
    if (q == first_)
    {
      first_ = q->next_;
      q->next_ = 0;
      return;
    }

    for (timer_queue_base* p = first_; p->next_; p = p->next_)
    {
      if (p->next_ == q)
      {
        p->next_ = q->next_;
        q->next_ = 0;
        return;
      }
    }
  }
}

bool timer_queue_set::all_empty() const
{
  for (timer_queue_base* p = first_; p; p = p->next_)
    if (!p->empty())
      return false;
  return true;
}

long timer_queue_set::wait_duration_msec(long max_duration) const
{
  long min_duration = max_duration;
  for (timer_queue_base* p = first_; p; p = p->next_)
    min_duration = p->wait_duration_msec(min_duration);
  return min_duration;
}

long timer_queue_set::wait_duration_usec(long max_duration) const
{
  long min_duration = max_duration;
  for (timer_queue_base* p = first_; p; p = p->next_)
    min_duration = p->wait_duration_usec(min_duration);
  return min_duration;
}

void timer_queue_set::get_ready_timers(op_queue<operation>& ops)
{
  for (timer_queue_base* p = first_; p; p = p->next_)
    p->get_ready_timers(ops);
}

void timer_queue_set::get_all_timers(op_queue<operation>& ops)
{
  for (timer_queue_base* p = first_; p; p = p->next_)
    p->get_all_timers(ops);
}

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_IMPL_TIMER_QUEUE_SET_IPP
