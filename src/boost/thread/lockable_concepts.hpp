// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_LOCKABLE_CONCEPTS_HPP
#define BOOST_THREAD_LOCKABLE_CONCEPTS_HPP

#include <boost/chrono/chrono.hpp>
#include <boost/concept_check.hpp>

namespace boost
{

  /**
   * BasicLockable object supports the basic features
   * required to delimit a critical region
   * Supports the basic lock and unlock functions.
   */

  //[BasicLockable
  template <typename Mutex>
  struct BasicLockable
  {

    BOOST_CONCEPT_USAGE(BasicLockable)
    {
      l.lock();
      l.unlock();
    }
    BasicLockable() : l(*static_cast<Mutex*>(0)) {}
  private:
    BasicLockable operator=(BasicLockable const&);

    Mutex& l;
  }
  ;
  //]
  /**
   * Lockable extends BasicLockable
   * with try_lock functions.
   */

  //[Lockable
  template <typename Mutex>
  struct Lockable
  {
    BOOST_CONCEPT_ASSERT(( BasicLockable<Mutex> ));

    BOOST_CONCEPT_USAGE(Lockable)
    {
      if (l.try_lock()) return;
    }
    Lockable() : l(*static_cast<Mutex*>(0)) {}
  private:
    Lockable operator=(Lockable const&);
    Mutex& l;
  };
  //]

  /**
   * TimedLockable object extends Lockable
   * with timed lock functions: try_lock_until and try_lock_for and the exception based lock_until and lock_for
   */

  //[TimedLockable
  template <typename Mutex>
  struct TimedLockable
  {
    BOOST_CONCEPT_ASSERT(( Lockable<Mutex> ));

    BOOST_CONCEPT_USAGE(TimedLockable)
    {
      if (l.try_lock_until(t)) return;
      if (l.try_lock_for(d)) return;
    }
    TimedLockable() : l(*static_cast<Mutex*>(0)) {}
  private:
    TimedLockable operator=(TimedLockable const&);
    Mutex& l;
    chrono::system_clock::time_point t;
    chrono::system_clock::duration d;
  };
  //]

  /**
   * SharedLockable object extends TimedLockable
   * with the lock_shared, lock_shared_until, lock_shared_for, try_lock_shared_until, try_lock_shared
   * and unlock_shared functions
   */
  //[SharedLockable
  template <typename Mutex>
  struct SharedLockable
  {
    BOOST_CONCEPT_ASSERT(( TimedLockable<Mutex> ));

    BOOST_CONCEPT_USAGE(SharedLockable)
    {
      l.lock_shared();
      l.unlock_shared();
      if (l.try_lock_shared()) return;
      if (l.try_lock_shared_until(t)) return;
      if (l.try_lock_shared_for(d)) return;
    }
    SharedLockable() : l(*static_cast<Mutex*>(0)) {}
  private:
    SharedLockable operator=(SharedLockable const&);
    Mutex& l;
    chrono::system_clock::time_point t;
    chrono::system_clock::duration d;
  };
  //]

  /**
   * UpgradeLockable object extends SharedLockable
   * with the lock_upgrade, lock_upgrade_until, unlock_upgrade_and_lock,
   * unlock_and_lock_shared and unlock_upgrade_and_lock_shared functions
   */

  //[UpgradeLockable
  template <typename Mutex>
  struct UpgradeLockable
  {
    BOOST_CONCEPT_ASSERT(( SharedLockable<Mutex> ));

    BOOST_CONCEPT_USAGE(UpgradeLockable)
    {
      l.lock_upgrade();
      l.unlock_upgrade();
      if (l.try_lock_upgrade()) return;
      if (l.try_lock_upgrade_until(t)) return;
      if (l.try_lock_upgrade_for(d)) return;
      if (l.try_unlock_shared_and_lock()) return;
      if (l.try_unlock_shared_and_lock_until(t)) return;
      if (l.try_unlock_shared_and_lock_for(d)) return;
      l.unlock_and_lock_shared();
      if (l.try_unlock_shared_and_lock_upgrade()) return;
      if (l.try_unlock_shared_and_lock_upgrade_until(t)) return;
      if (l.try_unlock_shared_and_lock_upgrade_for(d)) return;
      l.unlock_and_lock_upgrade();
      l.unlock_upgrade_and_lock();
      if (l.try_unlock_upgrade_and_lock()) return;
      if (l.try_unlock_upgrade_and_lock_until(t)) return;
      if (l.try_unlock_upgrade_and_lock_for(d)) return;
      l.unlock_upgrade_and_lock_shared();
    }
    UpgradeLockable() : l(*static_cast<Mutex*>(0)) {}
  private:
    UpgradeLockable operator=(UpgradeLockable const&);
    Mutex& l;
    chrono::system_clock::time_point t;
    chrono::system_clock::duration d;
  };
  //]

}
#endif
