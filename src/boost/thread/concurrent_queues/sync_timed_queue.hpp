// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014-2017 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_SYNC_TIMED_QUEUE_HPP
#define BOOST_THREAD_SYNC_TIMED_QUEUE_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/concurrent_queues/sync_priority_queue.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/chrono_io.hpp>

#include <algorithm> // std::min

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{
  // fixme: shouldn't the timepoint be configurable
  template <class T, class Clock = chrono::steady_clock, class TimePoint=typename Clock::time_point>
  struct scheduled_type
  {
    typedef T value_type;
    typedef Clock clock;
    typedef TimePoint time_point;
    T data;
    time_point time;

    BOOST_THREAD_COPYABLE_AND_MOVABLE(scheduled_type)

    scheduled_type(T const& pdata, time_point tp) : data(pdata), time(tp) {}
    scheduled_type(BOOST_THREAD_RV_REF(T) pdata, time_point tp) : data(boost::move(pdata)), time(tp) {}

    scheduled_type(scheduled_type const& other) : data(other.data), time(other.time) {}
    scheduled_type& operator=(BOOST_THREAD_COPY_ASSIGN_REF(scheduled_type) other) {
      data = other.data;
      time = other.time;
      return *this;
    }

    scheduled_type(BOOST_THREAD_RV_REF(scheduled_type) other) : data(boost::move(other.data)), time(other.time) {}
    scheduled_type& operator=(BOOST_THREAD_RV_REF(scheduled_type) other) {
      data = boost::move(other.data);
      time = other.time;
      return *this;
    }

    bool operator <(const scheduled_type & other) const
    {
      return this->time > other.time;
    }
  }; //end struct

  template <class Duration>
  chrono::time_point<chrono::steady_clock,Duration>
  limit_timepoint(chrono::time_point<chrono::steady_clock,Duration> const& tp)
  {
    // Clock == chrono::steady_clock
    return tp;
  }

  template <class Clock, class Duration>
  chrono::time_point<Clock,Duration>
  limit_timepoint(chrono::time_point<Clock,Duration> const& tp)
  {
    // Clock != chrono::steady_clock
    // The system time may jump while wait_until() is waiting. To compensate for this and time out near
    // the correct time, we limit how long wait_until() can wait before going around the loop again.
    const chrono::time_point<Clock,Duration> tpmax(chrono::time_point_cast<Duration>(Clock::now() + chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS)));
    return (std::min)(tp, tpmax);
  }

  template <class Duration>
  chrono::steady_clock::time_point
  convert_to_steady_clock_timepoint(chrono::time_point<chrono::steady_clock,Duration> const& tp)
  {
    // Clock == chrono::steady_clock
    return chrono::time_point_cast<chrono::steady_clock::duration>(tp);
  }

  template <class Clock, class Duration>
  chrono::steady_clock::time_point
  convert_to_steady_clock_timepoint(chrono::time_point<Clock,Duration> const& tp)
  {
    // Clock != chrono::steady_clock
    // The system time may jump while wait_until() is waiting. To compensate for this and time out near
    // the correct time, we limit how long wait_until() can wait before going around the loop again.
    const chrono::steady_clock::duration dura(chrono::duration_cast<chrono::steady_clock::duration>(tp - Clock::now()));
    const chrono::steady_clock::duration duramax(chrono::milliseconds(BOOST_THREAD_POLL_INTERVAL_MILLISECONDS));
    return chrono::steady_clock::now() + (std::min)(dura, duramax);
  }

} //end detail namespace

  template <class T, class Clock = chrono::steady_clock, class TimePoint=typename Clock::time_point>
  class sync_timed_queue
    : private sync_priority_queue<detail::scheduled_type<T, Clock, TimePoint> >
  {
    typedef detail::scheduled_type<T, Clock, TimePoint> stype;
    typedef sync_priority_queue<stype> super;
  public:
    typedef T value_type;
    typedef Clock clock;
    typedef typename clock::duration duration;
    typedef typename clock::time_point time_point;
    typedef typename super::underlying_queue_type underlying_queue_type;
    typedef typename super::size_type size_type;
    typedef typename super::op_status op_status;

    sync_timed_queue() : super() {};
    ~sync_timed_queue() {}

    using super::size;
    using super::empty;
    using super::full;
    using super::close;
    using super::closed;

    T pull();
    void pull(T& elem);

    template <class Duration>
    queue_op_status pull_until(chrono::time_point<clock,Duration> const& tp, T& elem);
    template <class Rep, class Period>
    queue_op_status pull_for(chrono::duration<Rep,Period> const& dura, T& elem);

    queue_op_status try_pull(T& elem);
    queue_op_status wait_pull(T& elem);
    queue_op_status nonblocking_pull(T& elem);

    template <class Duration>
    void push(const T& elem, chrono::time_point<clock,Duration> const& tp);
    template <class Rep, class Period>
    void push(const T& elem, chrono::duration<Rep,Period> const& dura);

    template <class Duration>
    void push(BOOST_THREAD_RV_REF(T) elem, chrono::time_point<clock,Duration> const& tp);
    template <class Rep, class Period>
    void push(BOOST_THREAD_RV_REF(T) elem, chrono::duration<Rep,Period> const& dura);

    template <class Duration>
    queue_op_status try_push(const T& elem, chrono::time_point<clock,Duration> const& tp);
    template <class Rep, class Period>
    queue_op_status try_push(const T& elem, chrono::duration<Rep,Period> const& dura);

    template <class Duration>
    queue_op_status try_push(BOOST_THREAD_RV_REF(T) elem, chrono::time_point<clock,Duration> const& tp);
    template <class Rep, class Period>
    queue_op_status try_push(BOOST_THREAD_RV_REF(T) elem, chrono::duration<Rep,Period> const& dura);

  private:
    inline bool not_empty_and_time_reached(unique_lock<mutex>& lk) const;
    inline bool not_empty_and_time_reached(lock_guard<mutex>& lk) const;

    bool wait_to_pull(unique_lock<mutex>&);
    queue_op_status wait_to_pull_until(unique_lock<mutex>&, TimePoint const& tp);
    template <class Rep, class Period>
    queue_op_status wait_to_pull_for(unique_lock<mutex>& lk, chrono::duration<Rep,Period> const& dura);

    T pull(unique_lock<mutex>&);
    T pull(lock_guard<mutex>&);

    void pull(unique_lock<mutex>&, T& elem);
    void pull(lock_guard<mutex>&, T& elem);

    queue_op_status try_pull(unique_lock<mutex>&, T& elem);
    queue_op_status try_pull(lock_guard<mutex>&, T& elem);

    queue_op_status wait_pull(unique_lock<mutex>& lk, T& elem);

    sync_timed_queue(const sync_timed_queue&);
    sync_timed_queue& operator=(const sync_timed_queue&);
    sync_timed_queue(BOOST_THREAD_RV_REF(sync_timed_queue));
    sync_timed_queue& operator=(BOOST_THREAD_RV_REF(sync_timed_queue));
  }; //end class


  template <class T, class Clock, class TimePoint>
  template <class Duration>
  void sync_timed_queue<T, Clock, TimePoint>::push(const T& elem, chrono::time_point<clock,Duration> const& tp)
  {
    super::push(stype(elem,tp));
  }

  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  void sync_timed_queue<T, Clock, TimePoint>::push(const T& elem, chrono::duration<Rep,Period> const& dura)
  {
    push(elem, clock::now() + dura);
  }

  template <class T, class Clock, class TimePoint>
  template <class Duration>
  void sync_timed_queue<T, Clock, TimePoint>::push(BOOST_THREAD_RV_REF(T) elem, chrono::time_point<clock,Duration> const& tp)
  {
    super::push(stype(boost::move(elem),tp));
  }

  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  void sync_timed_queue<T, Clock, TimePoint>::push(BOOST_THREAD_RV_REF(T) elem, chrono::duration<Rep,Period> const& dura)
  {
    push(boost::move(elem), clock::now() + dura);
  }



  template <class T, class Clock, class TimePoint>
  template <class Duration>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_push(const T& elem, chrono::time_point<clock,Duration> const& tp)
  {
    return super::try_push(stype(elem,tp));
  }

  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_push(const T& elem, chrono::duration<Rep,Period> const& dura)
  {
    return try_push(elem,clock::now() + dura);
  }

  template <class T, class Clock, class TimePoint>
  template <class Duration>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_push(BOOST_THREAD_RV_REF(T) elem, chrono::time_point<clock,Duration> const& tp)
  {
    return super::try_push(stype(boost::move(elem), tp));
  }

  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_push(BOOST_THREAD_RV_REF(T) elem, chrono::duration<Rep,Period> const& dura)
  {
    return try_push(boost::move(elem), clock::now() + dura);
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  bool sync_timed_queue<T, Clock, TimePoint>::not_empty_and_time_reached(unique_lock<mutex>& lk) const
  {
    return ! super::empty(lk) && clock::now() >= super::data_.top().time;
  }

  template <class T, class Clock, class TimePoint>
  bool sync_timed_queue<T, Clock, TimePoint>::not_empty_and_time_reached(lock_guard<mutex>& lk) const
  {
    return ! super::empty(lk) && clock::now() >= super::data_.top().time;
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  bool sync_timed_queue<T, Clock, TimePoint>::wait_to_pull(unique_lock<mutex>& lk)
  {
    for (;;)
    {
      if (not_empty_and_time_reached(lk)) return false; // success
      if (super::closed(lk)) return true; // closed

      super::wait_until_not_empty_or_closed(lk);

      if (not_empty_and_time_reached(lk)) return false; // success
      if (super::closed(lk)) return true; // closed

      const time_point tpmin(detail::limit_timepoint(super::data_.top().time));
      super::cond_.wait_until(lk, tpmin);
    }
  }

  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::wait_to_pull_until(unique_lock<mutex>& lk, TimePoint const& tp)
  {
    for (;;)
    {
      if (not_empty_and_time_reached(lk)) return queue_op_status::success;
      if (super::closed(lk)) return queue_op_status::closed;
      if (clock::now() >= tp) return super::empty(lk) ? queue_op_status::timeout : queue_op_status::not_ready;

      super::wait_until_not_empty_or_closed_until(lk, tp);

      if (not_empty_and_time_reached(lk)) return queue_op_status::success;
      if (super::closed(lk)) return queue_op_status::closed;
      if (clock::now() >= tp) return super::empty(lk) ? queue_op_status::timeout : queue_op_status::not_ready;

      const time_point tpmin((std::min)(tp, detail::limit_timepoint(super::data_.top().time)));
      super::cond_.wait_until(lk, tpmin);
    }
  }

  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::wait_to_pull_for(unique_lock<mutex>& lk, chrono::duration<Rep,Period> const& dura)
  {
    const chrono::steady_clock::time_point tp(chrono::steady_clock::now() + chrono::duration_cast<chrono::steady_clock::duration>(dura));
    for (;;)
    {
      if (not_empty_and_time_reached(lk)) return queue_op_status::success;
      if (super::closed(lk)) return queue_op_status::closed;
      if (chrono::steady_clock::now() >= tp) return super::empty(lk) ? queue_op_status::timeout : queue_op_status::not_ready;

      super::wait_until_not_empty_or_closed_until(lk, tp);

      if (not_empty_and_time_reached(lk)) return queue_op_status::success;
      if (super::closed(lk)) return queue_op_status::closed;
      if (chrono::steady_clock::now() >= tp) return super::empty(lk) ? queue_op_status::timeout : queue_op_status::not_ready;

      const chrono::steady_clock::time_point tpmin((std::min)(tp, detail::convert_to_steady_clock_timepoint(super::data_.top().time)));
      super::cond_.wait_until(lk, tpmin);
    }
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  T sync_timed_queue<T, Clock, TimePoint>::pull(unique_lock<mutex>&)
  {
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    return boost::move(super::data_.pull().data);
#else
    return super::data_.pull().data;
#endif
  }

  template <class T, class Clock, class TimePoint>
  T sync_timed_queue<T, Clock, TimePoint>::pull(lock_guard<mutex>&)
  {
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    return boost::move(super::data_.pull().data);
#else
    return super::data_.pull().data;
#endif
  }
  template <class T, class Clock, class TimePoint>
  T sync_timed_queue<T, Clock, TimePoint>::pull()
  {
    unique_lock<mutex> lk(super::mtx_);
    const bool has_been_closed = wait_to_pull(lk);
    if (has_been_closed) super::throw_if_closed(lk);
    return pull(lk);
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  void sync_timed_queue<T, Clock, TimePoint>::pull(unique_lock<mutex>&, T& elem)
  {
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    elem = boost::move(super::data_.pull().data);
#else
    elem = super::data_.pull().data;
#endif
  }

  template <class T, class Clock, class TimePoint>
  void sync_timed_queue<T, Clock, TimePoint>::pull(lock_guard<mutex>&, T& elem)
  {
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
    elem = boost::move(super::data_.pull().data);
#else
    elem = super::data_.pull().data;
#endif
  }

  template <class T, class Clock, class TimePoint>
  void sync_timed_queue<T, Clock, TimePoint>::pull(T& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    const bool has_been_closed = wait_to_pull(lk);
    if (has_been_closed) super::throw_if_closed(lk);
    pull(lk, elem);
  }

  //////////////////////
  template <class T, class Clock, class TimePoint>
  template <class Duration>
  queue_op_status
  sync_timed_queue<T, Clock, TimePoint>::pull_until(chrono::time_point<clock,Duration> const& tp, T& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    const queue_op_status rc = wait_to_pull_until(lk, chrono::time_point_cast<typename time_point::duration>(tp));
    if (rc == queue_op_status::success) pull(lk, elem);
    return rc;
  }

  //////////////////////
  template <class T, class Clock, class TimePoint>
  template <class Rep, class Period>
  queue_op_status
  sync_timed_queue<T, Clock, TimePoint>::pull_for(chrono::duration<Rep,Period> const& dura, T& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    const queue_op_status rc = wait_to_pull_for(lk, dura);
    if (rc == queue_op_status::success) pull(lk, elem);
    return rc;
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_pull(unique_lock<mutex>& lk, T& elem)
  {
    if (not_empty_and_time_reached(lk))
    {
      pull(lk, elem);
      return queue_op_status::success;
    }
    if (super::closed(lk)) return queue_op_status::closed;
    if (super::empty(lk)) return queue_op_status::empty;
    return queue_op_status::not_ready;
  }
  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_pull(lock_guard<mutex>& lk, T& elem)
  {
    if (not_empty_and_time_reached(lk))
    {
      pull(lk, elem);
      return queue_op_status::success;
    }
    if (super::closed(lk)) return queue_op_status::closed;
    if (super::empty(lk)) return queue_op_status::empty;
    return queue_op_status::not_ready;
  }

  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::try_pull(T& elem)
  {
    lock_guard<mutex> lk(super::mtx_);
    return try_pull(lk, elem);
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::wait_pull(unique_lock<mutex>& lk, T& elem)
  {
    const bool has_been_closed = wait_to_pull(lk);
    if (has_been_closed) return queue_op_status::closed;
    pull(lk, elem);
    return queue_op_status::success;
  }

  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::wait_pull(T& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_pull(lk, elem);
  }

  ///////////////////////////
  template <class T, class Clock, class TimePoint>
  queue_op_status sync_timed_queue<T, Clock, TimePoint>::nonblocking_pull(T& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (! lk.owns_lock()) return queue_op_status::busy;
    return try_pull(lk, elem);
  }

} //end concurrent namespace

using concurrent::sync_timed_queue;

} //end boost namespace
#include <boost/config/abi_suffix.hpp>

#endif
