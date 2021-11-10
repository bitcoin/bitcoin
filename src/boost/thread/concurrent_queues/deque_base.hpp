#ifndef BOOST_THREAD_CONCURRENT_DEQUE_BASE_HPP
#define BOOST_THREAD_CONCURRENT_DEQUE_BASE_HPP

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
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/is_copy_constructible.hpp>


#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace concurrent
{
namespace detail
{

  template <typename ValueType, class SizeType>
  class deque_base_copyable_only
  {
  public:
    typedef ValueType value_type;
    typedef SizeType size_type;

    // Constructors/Assignment/Destructors
    virtual ~deque_base_copyable_only() {};

    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void push_back(const value_type& x) = 0;

    virtual void pull_front(value_type&) = 0;
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_push_back(const value_type& x) = 0;
    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_push_back(const value_type& x) = 0;
    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_push_back(const value_type& x) = 0;
    virtual queue_op_status wait_pull_front(value_type& elem) = 0;

  };

  template <typename ValueType, class SizeType>
  class deque_base_movable_only
  {
  public:
    typedef ValueType value_type;
    typedef SizeType size_type;
    // Constructors/Assignment/Destructors
    virtual ~deque_base_movable_only() {};

    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void pull_front(value_type&) = 0;
    // enable_if is_nothrow_movable<value_type>
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_pull_front(value_type& elem) = 0;

    virtual void push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
  };


  template <typename ValueType, class SizeType>
  class deque_base_copyable_and_movable
  {
  public:
    typedef ValueType value_type;
    typedef SizeType size_type;
    // Constructors/Assignment/Destructors
    virtual ~deque_base_copyable_and_movable() {};


    // Observers
    virtual bool empty() const = 0;
    virtual bool full() const = 0;
    virtual size_type size() const = 0;
    virtual bool closed() const = 0;

    // Modifiers
    virtual void close() = 0;

    virtual void push_back(const value_type& x) = 0;

    virtual void pull_front(value_type&) = 0;
    // enable_if is_nothrow_copy_movable<value_type>
    virtual value_type pull_front() = 0;

    virtual queue_op_status try_push_back(const value_type& x) = 0;
    virtual queue_op_status try_pull_front(value_type&) = 0;

    virtual queue_op_status nonblocking_push_back(const value_type& x) = 0;
    virtual queue_op_status nonblocking_pull_front(value_type&) = 0;

    virtual queue_op_status wait_push_back(const value_type& x) = 0;
    virtual queue_op_status wait_pull_front(value_type& elem) = 0;

    virtual void push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status try_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status nonblocking_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
    virtual queue_op_status wait_push_back(BOOST_THREAD_RV_REF(value_type) x) = 0;
  };

  template <class T, class ST,
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
  struct deque_base;

  template <class T, class ST>
  struct deque_base<T, ST, true, true> {
    typedef deque_base_copyable_and_movable<T, ST> type;
  };
  template <class T, class ST>
  struct deque_base<T, ST, true, false> {
    typedef deque_base_copyable_only<T, ST> type;
  };
  template <class T, class ST>
  struct deque_base<T, ST, false, true> {
    typedef deque_base_movable_only<T, ST> type;
  };

}

  template <class ValueType, class SizeType=std::size_t>
  class deque_base :
    public detail::deque_base<ValueType, SizeType>::type
  {
  public:
      typedef ValueType value_type;
      typedef SizeType size_type;
    // Constructors/Assignment/Destructors
    virtual ~deque_base() {};
  };

}
using concurrent::deque_base;

}

#include <boost/config/abi_suffix.hpp>

#endif
