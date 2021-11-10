#ifndef BOOST_THREAD_CONCURRENT_QUEUES_SYNC_DEQUE_HPP
#define BOOST_THREAD_CONCURRENT_QUEUES_SYNC_DEQUE_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2013-2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/concurrent_queues/detail/sync_queue_base.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/csbl/devector.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/mutex.hpp>

#include <boost/throw_exception.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
  template <class ValueType, class Container = csbl::devector<ValueType> >
  class sync_deque
    : public detail::sync_queue_base<ValueType, Container >
  {
    typedef detail::sync_queue_base<ValueType, Container >  super;

  public:
    typedef ValueType value_type;
    //typedef typename super::value_type value_type; // fixme
    typedef typename super::underlying_queue_type underlying_queue_type;
    typedef typename super::size_type size_type;
    typedef typename super::op_status op_status;

    // Constructors/Assignment/Destructors
    BOOST_THREAD_NO_COPYABLE(sync_deque)
    inline sync_deque();
    //template <typename Range>
    //inline explicit sync_deque(Range range);
    inline ~sync_deque();

    // Modifiers
    inline void push_back(const value_type& x);
    inline queue_op_status try_push_back(const value_type& x);
    inline queue_op_status nonblocking_push_back(const value_type& x);
    inline queue_op_status wait_push_back(const value_type& x);
    inline void push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x);
    inline queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x);

    // Observers/Modifiers
    inline void pull_front(value_type&);
    // enable_if is_nothrow_copy_movable<value_type>
    inline value_type pull_front();

    inline queue_op_status try_pull_front(value_type&);
    inline queue_op_status nonblocking_pull_front(value_type&);
    inline queue_op_status wait_pull_front(ValueType& elem);

  private:

    inline queue_op_status try_pull_front(value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status wait_pull_front(value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status try_push_back(const value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status wait_push_back(const value_type& x, unique_lock<mutex>& lk);
    inline queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);
    inline queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x, unique_lock<mutex>& lk);

    inline void pull_front(value_type& elem, unique_lock<mutex>& )
    {
      elem = boost::move(super::data_.front());
      super::data_.pop_front();
    }
    inline value_type pull_front(unique_lock<mutex>& )
    {
      value_type e = boost::move(super::data_.front());
      super::data_.pop_front();
      return boost::move(e);
    }

    inline void push_back(const value_type& elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(elem);
      super::notify_elem_added(lk);
    }

    inline void push_back(BOOST_THREAD_RV_REF(value_type) elem, unique_lock<mutex>& lk)
    {
      super::data_.push_back(boost::move(elem));
      super::notify_elem_added(lk);
    }
  };

  template <class ValueType, class Container>
  sync_deque<ValueType, Container>::sync_deque() :
    super()
  {
  }

//  template <class ValueType, class Container>
//  template <class Range>
//  explicit sync_deque<ValueType, Container>::sync_deque(Range range) :
//    data_(), closed_(false)
//  {
//    try
//    {
//      typedef typename Range::iterator iterator_t;
//      iterator_t first = boost::begin(range);
//      iterator_t end = boost::end(range);
//      for (iterator_t cur = first; cur != end; ++cur)
//      {
//        data_.push(boost::move(*cur));;
//      }
//      notify_elem_added(lk);
//    }
//    catch (...)
//    {
//      delete[] data_;
//    }
//  }

  template <class ValueType, class Container>
  sync_deque<ValueType, Container>::~sync_deque()
  {
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_pull_front(ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::empty(lk))
    {
      if (super::closed(lk)) return queue_op_status::closed;
      return queue_op_status::empty;
    }
    pull_front(elem, lk);
    return queue_op_status::success;
  }
  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_pull_front(ValueType& elem, unique_lock<mutex>& lk)
  {
    const bool has_been_closed = super::wait_until_not_empty_or_closed(lk);
    if (has_been_closed) return queue_op_status::closed;
    pull_front(elem, lk);
    return queue_op_status::success;
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_pull_front(elem, lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_pull_front(elem, lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::nonblocking_pull_front(ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock())
    {
      return queue_op_status::busy;
    }
    return try_pull_front(elem, lk);
  }

  template <class ValueType, class Container>
  void sync_deque<ValueType, Container>::pull_front(ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      const bool has_been_closed = super::wait_until_not_empty_or_closed(lk);
      if (has_been_closed) super::throw_if_closed(lk);
      pull_front(elem, lk);
  }

  // enable if ValueType is nothrow movable
  template <class ValueType, class Container>
  ValueType sync_deque<ValueType, Container>::pull_front()
  {
      unique_lock<mutex> lk(super::mtx_);
      const bool has_been_closed = super::wait_until_not_empty_or_closed(lk);
      if (has_been_closed) super::throw_if_closed(lk);
      return pull_front(lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_push_back(const ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(elem, lk);
    return queue_op_status::success;
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_push_back(elem, lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_push_back(const ValueType& elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(elem, lk);
    return queue_op_status::success;
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_push_back(elem, lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::nonblocking_push_back(const ValueType& elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock()) return queue_op_status::busy;
    return try_push_back(elem, lk);
  }

  template <class ValueType, class Container>
  void sync_deque<ValueType, Container>::push_back(const ValueType& elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push_back(elem, lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_push_back(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(boost::move(elem), lk);
    return queue_op_status::success;
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::try_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return try_push_back(boost::move(elem), lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_push_back(BOOST_THREAD_RV_REF(ValueType) elem, unique_lock<mutex>& lk)
  {
    if (super::closed(lk)) return queue_op_status::closed;
    push_back(boost::move(elem), lk);
    return queue_op_status::success;
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::wait_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_);
    return wait_push_back(boost::move(elem), lk);
  }

  template <class ValueType, class Container>
  queue_op_status sync_deque<ValueType, Container>::nonblocking_push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
    unique_lock<mutex> lk(super::mtx_, try_to_lock);
    if (!lk.owns_lock())
    {
      return queue_op_status::busy;
    }
    return try_push_back(boost::move(elem), lk);
  }

  template <class ValueType, class Container>
  void sync_deque<ValueType, Container>::push_back(BOOST_THREAD_RV_REF(ValueType) elem)
  {
      unique_lock<mutex> lk(super::mtx_);
      super::throw_if_closed(lk);
      push_back(boost::move(elem), lk);
  }

  template <class ValueType, class Container>
  sync_deque<ValueType, Container>& operator<<(sync_deque<ValueType, Container>& sbq, BOOST_THREAD_RV_REF(ValueType) elem)
  {
    sbq.push_back(boost::move(elem));
    return sbq;
  }

  template <class ValueType, class Container>
  sync_deque<ValueType, Container>& operator<<(sync_deque<ValueType, Container>& sbq, ValueType const&elem)
  {
    sbq.push_back(elem);
    return sbq;
  }

  template <class ValueType, class Container>
  sync_deque<ValueType, Container>& operator>>(sync_deque<ValueType, Container>& sbq, ValueType &elem)
  {
    sbq.pull_front(elem);
    return sbq;
  }

}
using concurrent::sync_deque;

}

#include <boost/config/abi_suffix.hpp>

#endif
