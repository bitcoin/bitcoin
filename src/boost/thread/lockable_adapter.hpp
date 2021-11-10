//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_LOCKABLE_ADAPTER_HPP
#define BOOST_THREAD_LOCKABLE_ADAPTER_HPP

#include <boost/thread/detail/delete.hpp>
#include <boost/chrono/chrono.hpp>

namespace boost
{

  //[basic_lockable_adapter
  template <typename BasicLockable>
  class basic_lockable_adapter
  {
  public:
    typedef BasicLockable mutex_type;

  protected:
    mutex_type& lockable() const
    {
      return lockable_;
    }
    mutable mutex_type lockable_; /*< mutable so that it can be modified by const functions >*/
  public:

    BOOST_THREAD_NO_COPYABLE( basic_lockable_adapter) /*< no copyable >*/

    basic_lockable_adapter()
    {}

    void lock() const
    {
      lockable().lock();
    }
    void unlock() const
    {
      lockable().unlock();
    }

  };
  //]

  //[lockable_adapter
  template <typename Lockable>
  class lockable_adapter : public basic_lockable_adapter<Lockable>
  {
  public:
    typedef Lockable mutex_type;

    bool try_lock() const
    {
      return this->lockable().try_lock();
    }
  };
  //]

  //[timed_lockable_adapter
  template <typename TimedLock>
  class timed_lockable_adapter: public lockable_adapter<TimedLock>
  {
  public:
    typedef TimedLock mutex_type;

    template <typename Clock, typename Duration>
    bool try_lock_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_lock_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_lock_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_lock_for(rel_time);
    }

  };
  //]

  //[shared_lockable_adapter
  template <typename SharableLock>
  class shared_lockable_adapter: public timed_lockable_adapter<SharableLock>
  {
  public:
    typedef SharableLock mutex_type;

    void lock_shared() const
    {
      this->lockable().lock_shared();
    }
    bool try_lock_shared() const
    {
      return this->lockable().try_lock_shared();
    }
    void unlock_shared() const
    {
      this->lockable().unlock_shared();
    }

    template <typename Clock, typename Duration>
    bool try_lock_shared_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_lock_shared_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_lock_shared_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_lock_shared_for(rel_time);
    }

  };

  //]

  //[upgrade_lockable_adapter
  template <typename UpgradableLock>
  class upgrade_lockable_adapter: public shared_lockable_adapter<UpgradableLock>
  {
  public:
    typedef UpgradableLock mutex_type;

    void lock_upgrade() const
    {
      this->lockable().lock_upgrade();
    }

    bool try_lock_upgrade() const
    {
      return this->lockable().try_lock_upgrade();
    }

    void unlock_upgrade() const
    {
      this->lockable().unlock_upgrade();
    }

    template <typename Clock, typename Duration>
    bool try_lock_upgrade_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_lock_upgrade_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_lock_upgrade_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_lock_upgrade_for(rel_time);
    }

    bool try_unlock_shared_and_lock() const
    {
      return this->lockable().try_unlock_shared_and_lock();
    }

    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_unlock_shared_and_lock_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_unlock_shared_and_lock_for(rel_time);
    }

    void unlock_and_lock_shared() const
    {
      this->lockable().unlock_and_lock_shared();
    }

    bool try_unlock_shared_and_lock_upgrade() const
    {
      return this->lockable().try_unlock_shared_and_lock_upgrade();
    }

    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_upgrade_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_unlock_shared_and_lock_upgrade_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_upgrade_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_unlock_shared_and_lock_upgrade_for(rel_time);
    }

    void unlock_and_lock_upgrade() const
    {
      this->lockable().unlock_and_lock_upgrade();
    }

    void unlock_upgrade_and_lock() const
    {
      this->lockable().unlock_upgrade_and_lock();
    }

    bool try_unlock_upgrade_and_lock() const
    {
      return this->lockable().try_unlock_upgrade_and_lock();
    }
    template <typename Clock, typename Duration>
    bool try_unlock_upgrade_and_lock_until(chrono::time_point<Clock, Duration> const & abs_time) const
    {
      return this->lockable().try_unlock_upgrade_and_lock_until(abs_time);
    }
    template <typename Rep, typename Period>
    bool try_unlock_upgrade_and_lock_for(chrono::duration<Rep, Period> const & rel_time) const
    {
      return this->lockable().try_unlock_upgrade_and_lock_for(rel_time);
    }

    void unlock_upgrade_and_lock_shared() const
    {
      this->lockable().unlock_upgrade_and_lock_shared();
    }

  };
//]

}
#endif
