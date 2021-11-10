//
// detail/timer_queue_set.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_TIMER_QUEUE_SET_HPP
#define BOOST_ASIO_DETAIL_TIMER_QUEUE_SET_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/timer_queue_base.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

class timer_queue_set
{
public:
  // Constructor.
  BOOST_ASIO_DECL timer_queue_set();

  // Add a timer queue to the set.
  BOOST_ASIO_DECL void insert(timer_queue_base* q);

  // Remove a timer queue from the set.
  BOOST_ASIO_DECL void erase(timer_queue_base* q);

  // Determine whether all queues are empty.
  BOOST_ASIO_DECL bool all_empty() const;

  // Get the wait duration in milliseconds.
  BOOST_ASIO_DECL long wait_duration_msec(long max_duration) const;

  // Get the wait duration in microseconds.
  BOOST_ASIO_DECL long wait_duration_usec(long max_duration) const;

  // Dequeue all ready timers.
  BOOST_ASIO_DECL void get_ready_timers(op_queue<operation>& ops);

  // Dequeue all timers.
  BOOST_ASIO_DECL void get_all_timers(op_queue<operation>& ops);

private:
  timer_queue_base* first_;
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#if defined(BOOST_ASIO_HEADER_ONLY)
# include <boost/asio/detail/impl/timer_queue_set.ipp>
#endif // defined(BOOST_ASIO_HEADER_ONLY)

#endif // BOOST_ASIO_DETAIL_TIMER_QUEUE_SET_HPP
