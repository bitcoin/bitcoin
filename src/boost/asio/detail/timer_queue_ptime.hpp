//
// detail/timer_queue_ptime.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_TIMER_QUEUE_PTIME_HPP
#define BOOST_ASIO_DETAIL_TIMER_QUEUE_PTIME_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#if defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#include <boost/asio/time_traits.hpp>
#include <boost/asio/detail/timer_queue.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

struct forwarding_posix_time_traits : time_traits<boost::posix_time::ptime> {};

// Template specialisation for the commonly used instantation.
template <>
class timer_queue<time_traits<boost::posix_time::ptime> >
  : public timer_queue_base
{
public:
  // The time type.
  typedef boost::posix_time::ptime time_type;

  // The duration type.
  typedef boost::posix_time::time_duration duration_type;

  // Per-timer data.
  typedef timer_queue<forwarding_posix_time_traits>::per_timer_data
    per_timer_data;

  // Constructor.
  BOOST_ASIO_DECL timer_queue();

  // Destructor.
  BOOST_ASIO_DECL virtual ~timer_queue();

  // Add a new timer to the queue. Returns true if this is the timer that is
  // earliest in the queue, in which case the reactor's event demultiplexing
  // function call may need to be interrupted and restarted.
  BOOST_ASIO_DECL bool enqueue_timer(const time_type& time,
      per_timer_data& timer, wait_op* op);

  // Whether there are no timers in the queue.
  BOOST_ASIO_DECL virtual bool empty() const;

  // Get the time for the timer that is earliest in the queue.
  BOOST_ASIO_DECL virtual long wait_duration_msec(long max_duration) const;

  // Get the time for the timer that is earliest in the queue.
  BOOST_ASIO_DECL virtual long wait_duration_usec(long max_duration) const;

  // Dequeue all timers not later than the current time.
  BOOST_ASIO_DECL virtual void get_ready_timers(op_queue<operation>& ops);

  // Dequeue all timers.
  BOOST_ASIO_DECL virtual void get_all_timers(op_queue<operation>& ops);

  // Cancel and dequeue operations for the given timer.
  BOOST_ASIO_DECL std::size_t cancel_timer(
      per_timer_data& timer, op_queue<operation>& ops,
      std::size_t max_cancelled = (std::numeric_limits<std::size_t>::max)());

  // Cancel and dequeue operations for the given timer and key.
  BOOST_ASIO_DECL void cancel_timer_by_key(per_timer_data* timer,
      op_queue<operation>& ops, void* cancellation_key);

  // Move operations from one timer to another, empty timer.
  BOOST_ASIO_DECL void move_timer(per_timer_data& target,
      per_timer_data& source);

private:
  timer_queue<forwarding_posix_time_traits> impl_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/timer_queue_ptime.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // defined(BOOST_ASIO_HAS_BOOST_DATE_TIME)

#endif // BOOST_ASIO_DETAIL_TIMER_QUEUE_PTIME_HPP
