//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_POLY_SHARED_LOCKABLE_ADAPTER_HPP
#define BOOST_THREAD_POLY_SHARED_LOCKABLE_ADAPTER_HPP

#include <boost/thread/poly_lockable_adapter.hpp>
#include <boost/thread/poly_shared_lockable.hpp>

namespace boost
{

  //[poly_shared_lockable_adapter
  template <typename Mutex, typename Base=poly_shared_lockable>
  class poly_shared_lockable_adapter: public poly_timed_lockable_adapter<Mutex, Base>
  {
  public:
    typedef Mutex mutex_type;

    void lock_shared()
    {
      this->mtx().lock_shared();
    }
    bool try_lock_shared()
    {
      return this->mtx().try_lock_shared();
    }
    void unlock_shared()
    {
      this->mtx().unlock_shared();
    }

    bool try_lock_shared_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_shared_until(abs_time);
    }
    bool try_lock_shared_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_shared_until(abs_time);
    }
    bool try_lock_shared_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_lock_shared_for(rel_time);
    }

  };

  //]

  //[poly_upgrade_lockable_adapter
  template <typename Mutex, typename Base=poly_shared_lockable>
  class poly_upgrade_lockable_adapter: public poly_shared_lockable_adapter<Mutex, Base>
  {
  public:
    typedef Mutex mutex_type;

    void lock_upgrade()
    {
      this->mtx().lock_upgrade();
    }

    bool try_lock_upgrade()
    {
      return this->mtx().try_lock_upgrade();
    }

    void unlock_upgrade()
    {
      this->mtx().unlock_upgrade();
    }

    bool try_lock_upgrade_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_upgrade_until(abs_time);
    }
    bool try_lock_upgrade_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_lock_upgrade_until(abs_time);
    }
    bool try_lock_upgrade_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_lock_upgrade_for(rel_time);
    }

    bool try_unlock_shared_and_lock()
    {
      return this->mtx().try_unlock_shared_and_lock();
    }

    bool try_unlock_shared_and_lock_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_shared_and_lock_until(abs_time);
    }
    bool try_unlock_shared_and_lock_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_shared_and_lock_until(abs_time);
    }
    bool try_unlock_shared_and_lock_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_unlock_shared_and_lock_for(rel_time);
    }

    void unlock_and_lock_shared()
    {
      this->mtx().unlock_and_lock_shared();
    }

    bool try_unlock_shared_and_lock_upgrade()
    {
      return this->mtx().try_unlock_shared_and_lock_upgrade();
    }

    bool try_unlock_shared_and_lock_upgrade_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_shared_and_lock_upgrade_until(abs_time);
    }
    bool try_unlock_shared_and_lock_upgrade_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_shared_and_lock_upgrade_until(abs_time);
    }
    bool try_unlock_shared_and_lock_upgrade_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_unlock_shared_and_lock_upgrade_for(rel_time);
    }

    void unlock_and_lock_upgrade()
    {
      this->mtx().unlock_and_lock_upgrade();
    }

    void unlock_upgrade_and_lock()
    {
      this->mtx().unlock_upgrade_and_lock();
    }

    bool try_unlock_upgrade_and_lock()
    {
      return this->mtx().try_unlock_upgrade_and_lock();
    }
    bool try_unlock_upgrade_and_lock_until(chrono::system_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_upgrade_and_lock_until(abs_time);
    }
    bool try_unlock_upgrade_and_lock_until(chrono::steady_clock::time_point const & abs_time)
    {
      return this->mtx().try_unlock_upgrade_and_lock_until(abs_time);
    }
    bool try_unlock_upgrade_and_lock_for(chrono::nanoseconds const & rel_time)
    {
      return this->mtx().try_unlock_upgrade_and_lock_for(rel_time);
    }

    void unlock_upgrade_and_lock_shared()
    {
      this->mtx().unlock_upgrade_and_lock_shared();
    }

  };
//]

}
#endif
