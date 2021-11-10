//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_POLY_LOCKABLE_ADAPTER_HPP
#define BOOST_THREAD_POLY_LOCKABLE_ADAPTER_HPP

#include <boost/thread/poly_lockable.hpp>

namespace boost
{

  //[poly_basic_lockable_adapter
  template <typename Mutex, typename Base=poly_basic_lockable>
  class poly_basic_lockable_adapter : public Base
  {
  public:
    typedef Mutex mutex_type;

  protected:
    mutex_type& mtx() const
    {
      return mtx_;
    }
    mutable mutex_type mtx_; /*< mutable so that it can be modified by const functions >*/
  public:

    BOOST_THREAD_NO_COPYABLE( poly_basic_lockable_adapter) /*< no copyable >*/

    poly_basic_lockable_adapter()
    {}

    void lock()
    {
      mtx().lock();
    }
    void unlock()
    {
      mtx().unlock();
    }

  };
  //]

  //[poly_lockable_adapter
  template <typename Mutex, typename Base=poly_lockable>
  class poly_lockable_adapter : public poly_basic_lockable_adapter<Mutex, Base>
  {
  public:
    typedef Mutex mutex_type;

    bool try_lock()
    {
      return this->mtx().try_lock();
    }
  };
  //]

  //[poly_timed_lockable_adapter
  template <typename Mutex, typename Base=poly_timed_lockable>
  class poly_timed_lockable_adapter: public poly_lockable_adapter<Mutex, Base>
  {
  public:
    typedef Mutex mutex_type;

    bool try_lock_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_until(abs_time);
    }
    bool try_lock_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_until(abs_time);
    }
    bool try_lock_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_lock_for(rel_time);
    }

  };
  //]

}
#endif
