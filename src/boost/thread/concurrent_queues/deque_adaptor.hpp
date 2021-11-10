#ifndef BOOST_THREAD_CONCURRENT_DEQUE_ADAPTOR_HPP
#define BOOST_THREAD_CONCURRENT_DEQUE_ADAPTOR_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/concurrent_queues/queue_op_status.hpp>
#include <boost/thread/concurrent_queues/deque_base.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{

  template <typename Queue>
  class deque_adaptor_copyable_only :
    public boost::deque_base<typename Queue::value_type, typename Queue::size_type>
  {
      Queue queue;
  public:
    typedef typename Queue::value_type value_type;
    typedef typename Queue::size_type size_type;

    // Constructors/Assignment/Destructors
    deque_adaptor_copyable_only()  {}

    // Observers
    bool empty() const  { return queue.empty(); }
    bool full() const { return queue.full(); }
    size_type size() const { return queue.size(); }
    bool closed() const { return queue.closed(); }

    // Modifiers
    void close() { queue.close(); }

    void push_back(const value_type& x) { queue.push_back(x); }

    void pull_front(value_type& x) { queue.pull_front(x); };
    value_type pull_front() { return queue.pull_front(); }

    queue_op_status try_push_back(const value_type& x) { return queue.try_push_back(x); }
    queue_op_status try_pull_front(value_type& x)  { return queue.try_pull_front(x); }

    queue_op_status nonblocking_push_back(const value_type& x) { return queue.nonblocking_push_back(x); }
    queue_op_status nonblocking_pull_front(value_type& x)  { return queue.nonblocking_pull_front(x); }

    queue_op_status wait_push_back(const value_type& x) { return queue.wait_push_back(x); }
    queue_op_status wait_pull_front(value_type& x) { return queue.wait_pull_front(x); }

  };
  template <typename Queue>
  class deque_adaptor_movable_only :
    public boost::deque_base<typename Queue::value_type, typename Queue::size_type>
  {
      Queue queue;
  public:
    typedef typename Queue::value_type value_type;
    typedef typename Queue::size_type size_type;

    // Constructors/Assignment/Destructors

    deque_adaptor_movable_only()  {}

    // Observers
    bool empty() const  { return queue.empty(); }
    bool full() const { return queue.full(); }
    size_type size() const { return queue.size(); }
    bool closed() const { return queue.closed(); }

    // Modifiers
    void close() { queue.close(); }


    void pull_front(value_type& x) { queue.pull_front(x); };
    // enable_if is_nothrow_copy_movable<value_type>
    value_type pull_front() { return queue.pull_front(); }

    queue_op_status try_pull_front(value_type& x)  { return queue.try_pull_front(x); }

    queue_op_status nonblocking_pull_front(value_type& x)  { return queue.nonblocking_pull_front(x); }

    queue_op_status wait_pull_front(value_type& x) { return queue.wait_pull_front(x); }

    void push_back(BOOST_THREAD_RV_REF(value_type) x) { queue.push_back(boost::move(x)); }
    queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.try_push_back(boost::move(x)); }
    queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.nonblocking_push_back(boost::move(x)); }
    queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.wait_push_back(boost::move(x)); }
  };

  template <typename Queue>
  class deque_adaptor_copyable_and_movable :
    public boost::deque_base<typename Queue::value_type, typename Queue::size_type>
  {
      Queue queue;
  public:
    typedef typename Queue::value_type value_type;
    typedef typename Queue::size_type size_type;

    // Constructors/Assignment/Destructors

    deque_adaptor_copyable_and_movable()  {}

    // Observers
    bool empty() const  { return queue.empty(); }
    bool full() const { return queue.full(); }
    size_type size() const { return queue.size(); }
    bool closed() const { return queue.closed(); }

    // Modifiers
    void close() { queue.close(); }


    void push_back(const value_type& x) { queue.push_back(x); }

    void pull_front(value_type& x) { queue.pull_front(x); };
    // enable_if is_nothrow_copy_movable<value_type>
    value_type pull_front() { return queue.pull_front(); }

    queue_op_status try_push_back(const value_type& x) { return queue.try_push_back(x); }
    queue_op_status try_pull_front(value_type& x)  { return queue.try_pull_front(x); }

    queue_op_status nonblocking_push_back(const value_type& x) { return queue.nonblocking_push_back(x); }
    queue_op_status nonblocking_pull_front(value_type& x)  { return queue.nonblocking_pull_front(x); }

    queue_op_status wait_push_back(const value_type& x) { return queue.wait_push_back(x); }
    queue_op_status wait_pull_front(value_type& x) { return queue.wait_pull_front(x); }

    void push_back(BOOST_THREAD_RV_REF(value_type) x) { queue.push_back(boost::move(x)); }
    queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.try_push_back(boost::move(x)); }
    queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.nonblocking_push_back(boost::move(x)); }
    queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) { return queue.wait_push_back(boost::move(x)); }
  };


  template <class Q, class T,
#if ! defined  BOOST_NO_CXX11_RVALUE_REFERENCES
#if defined __GNUC__ && ! defined __clang__
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 7) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = true
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif // __GNUC__
#elif defined _MSC_VER
#if _MSC_VER < 1700
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = true
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif // _MSC_VER
#else
          bool Copyable = std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
          bool Movable = std::is_move_constructible<T>::value && std::is_move_assignable<T>::value
#endif
#else
          bool Copyable = is_copy_constructible<T>::value,
          bool Movable = has_move_emulation_enabled<T>::value
#endif
      >
  struct deque_adaptor;

  template <class Q, class T>
  struct deque_adaptor<Q, T, true, true> {
    typedef deque_adaptor_copyable_and_movable<Q> type;
  };
  template <class Q, class T>
  struct deque_adaptor<Q, T, true, false> {
    typedef deque_adaptor_copyable_only<Q> type;
  };
  template <class Q, class T>
  struct deque_adaptor<Q, T, false, true> {
    typedef deque_adaptor_movable_only<Q> type;
  };

}

  template <typename Queue>
  class deque_adaptor :
    public detail::deque_adaptor<Queue, typename Queue::value_type>::type
  {
  public:
      typedef typename Queue::value_type value_type;
      typedef typename Queue::size_type size_type;
    // Constructors/Assignment/Destructors
    virtual ~deque_adaptor() {};
  };
}
using concurrent::deque_adaptor;

}

#include <boost/config/abi_suffix.hpp>

#endif
