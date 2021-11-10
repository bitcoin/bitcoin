#ifndef BOOST_THREAD_CONCURRENT_QUEUES_DETAIL_SYNC_DEQUE_BASE_HPP
#define BOOST_THREAD_CONCURRENT_QUEUES_DETAIL_SYNC_DEQUE_BASE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2013-2017. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/bind/bind.hpp>

#include <boost/thread/detail/config.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>

#include <boost/chrono/time_point.hpp>
#include <boost/throw_exception.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{

  template <class ValueType, class Queue>
  class sync_deque_base
  {
  public:
    typedef ValueType value_type;
    typedef Queue underlying_queue_type;
    typedef typename Queue::size_type size_type;
    typedef queue_op_status op_status;

    // Constructors/Assignment/Destructors
    BOOST_THREAD_NO_COPYABLE(sync_deque_base)
    inline sync_deque_base();
    //template <typename Range>
    //inline explicit sync_deque(Range range);
    inline ~sync_deque_base();

    // Observers
    inline bool empty() const;
    inline bool full() const;
    inline size_type size() const;
    inline bool closed() const;

    // Modifiers
    inline void close();

    inline underlying_queue_type underlying_queue() {
      lock_guard<mutex> lk(mtx_);
      return boost::move(data_);
    }

  protected:
    mutable mutex mtx_;
    condition_variable cond_;
    underlying_queue_type data_;
    bool closed_;

    inline bool empty(unique_lock<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.empty();
    }
    inline bool empty(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.empty();
    }

    inline size_type size(lock_guard<mutex>& ) const BOOST_NOEXCEPT
    {
      return data_.size();
    }
    inline bool closed(unique_lock<mutex>& lk) const;
    inline bool closed(lock_guard<mutex>& lk) const;

    inline void throw_if_closed(unique_lock<mutex>&);
    inline void throw_if_closed(lock_guard<mutex>&);

    inline bool not_empty_or_closed(unique_lock<mutex>& ) const;

    inline bool wait_until_not_empty_or_closed(unique_lock<mutex>& lk);
    template <class WClock, class Duration>
    queue_op_status wait_until_not_empty_or_closed_until(unique_lock<mutex>& lk, chrono::time_point<WClock,Duration> const&tp);

    inline void notify_elem_added(unique_lock<mutex>& )
    {
      cond_.notify_all();
    }
    inline void notify_elem_added(lock_guard<mutex>& )
    {
      cond_.notify_all();
    }

  };

  template <class ValueType, class Queue>
  sync_deque_base<ValueType, Queue>::sync_deque_base() :
    data_(), closed_(false)
  {
    BOOST_ASSERT(data_.empty());
  }

  template <class ValueType, class Queue>
  sync_deque_base<ValueType, Queue>::~sync_deque_base()
  {
  }

  template <class ValueType, class Queue>
  void sync_deque_base<ValueType, Queue>::close()
  {
    {
      lock_guard<mutex> lk(mtx_);
      closed_ = true;
    }
    cond_.notify_all();
  }

  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::closed() const
  {
    lock_guard<mutex> lk(mtx_);
    return closed(lk);
  }
  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::closed(unique_lock<mutex>&) const
  {
    return closed_;
  }
  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::closed(lock_guard<mutex>&) const
  {
    return closed_;
  }

  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::empty() const
  {
    lock_guard<mutex> lk(mtx_);
    return empty(lk);
  }
  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::full() const
  {
    return false;
  }

  template <class ValueType, class Queue>
  typename sync_deque_base<ValueType, Queue>::size_type sync_deque_base<ValueType, Queue>::size() const
  {
    lock_guard<mutex> lk(mtx_);
    return size(lk);
  }

  template <class ValueType, class Queue>
  void sync_deque_base<ValueType, Queue>::throw_if_closed(unique_lock<mutex>& lk)
  {
    if (closed(lk))
    {
      BOOST_THROW_EXCEPTION( sync_deque_is_closed() );
    }
  }
  template <class ValueType, class Queue>
  void sync_deque_base<ValueType, Queue>::throw_if_closed(lock_guard<mutex>& lk)
  {
    if (closed(lk))
    {
      BOOST_THROW_EXCEPTION( sync_deque_is_closed() );
    }
  }

  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::not_empty_or_closed(unique_lock<mutex>& ) const
  {
    return ! data_.empty() || closed_;
  }

  template <class ValueType, class Queue>
  bool sync_deque_base<ValueType, Queue>::wait_until_not_empty_or_closed(unique_lock<mutex>& lk)
  {
    cond_.wait(lk, boost::bind(&sync_deque_base<ValueType, Queue>::not_empty_or_closed, boost::ref(*this), boost::ref(lk)));
    if (! empty(lk)) return false; // success
    return true; // closed
  }

  template <class ValueType, class Queue>
  template <class WClock, class Duration>
  queue_op_status sync_deque_base<ValueType, Queue>::wait_until_not_empty_or_closed_until(unique_lock<mutex>& lk, chrono::time_point<WClock,Duration> const&tp)
  {
    if (! cond_.wait_until(lk, tp, boost::bind(&sync_deque_base<ValueType, Queue>::not_empty_or_closed, boost::ref(*this), boost::ref(lk))))
      return queue_op_status::timeout;
    if (! empty(lk)) return queue_op_status::success;
    return queue_op_status::closed;
  }

} // detail
} // concurrent
} // boost

#include <boost/config/abi_suffix.hpp>

#endif
