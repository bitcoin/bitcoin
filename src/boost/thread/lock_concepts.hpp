// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_LOCK_CONCEPTS_HPP
#define BOOST_THREAD_LOCK_CONCEPTS_HPP

#include <boost/thread/lock_traits.hpp>
#include <boost/thread/lock_options.hpp>
#include <boost/thread/lockable_concepts.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/chrono/chrono.hpp>
#include <boost/concept_check.hpp>
#include <boost/static_assert.hpp>

namespace boost
{

  /**
   * BasicLock object supports the basic features
   * required to delimit a critical region
   * Supports the basic lock, unlock and try_lock functions and
   * defines the lock traits
   */

  template <typename Lk>
  struct BasicLock
  {
    typedef typename Lk::mutex_type mutex_type;
    void cvt_mutex_ptr(mutex_type*) {}
    BOOST_CONCEPT_ASSERT(( BasicLockable<mutex_type> ));

    BOOST_CONCEPT_USAGE(BasicLock)
    {
      const Lk l1(mtx);
      Lk l2(mtx, defer_lock);
      Lk l3(mtx, adopt_lock);
      Lk l4(( Lk()));
      Lk l5(( boost::move(l2)));
      cvt_mutex_ptr(l1.mutex());
      if (l1.owns_lock()) return;
      if (l1) return;
      if (!l1) return;

      l2.lock();
      l2.unlock();
      l2.release();

    }
    BasicLock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    BasicLock operator=(BasicLock const&);
    mutex_type& mtx;
  }
  ;

  template <typename Lk>
  struct Lock
  {
    BOOST_CONCEPT_ASSERT(( BasicLock<Lk> ));
    typedef typename Lk::mutex_type mutex_type;
    BOOST_CONCEPT_ASSERT(( Lockable<mutex_type> ));

    BOOST_CONCEPT_USAGE(Lock)
    {
      Lk l1(mtx, try_to_lock);
      if (l1.try_lock()) return;
    }
    Lock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    Lock operator=(Lock const&);
    mutex_type& mtx;
  };

  template <typename Lk>
  struct TimedLock
  {
    BOOST_CONCEPT_ASSERT(( Lock<Lk> ));
    typedef typename Lk::mutex_type mutex_type;
    BOOST_CONCEPT_ASSERT(( TimedLockable<mutex_type> ));

    BOOST_CONCEPT_USAGE(TimedLock)
    {
      const Lk l1(mtx, t);
      Lk l2(mtx, d);
      if (l1.try_lock_until(t)) return;
      if (l1.try_lock_for(d)) return;
    }
    TimedLock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    TimedLock operator=(TimedLock const&);
    mutex_type& mtx;
    boost::chrono::system_clock::time_point t;
    boost::chrono::system_clock::duration d;
  };

  template <typename Lk>
  struct UniqueLock
  {
    BOOST_CONCEPT_ASSERT(( TimedLock<Lk> ));
    typedef typename Lk::mutex_type mutex_type;

    BOOST_CONCEPT_USAGE(UniqueLock)
    {

    }
    UniqueLock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    UniqueLock operator=(UniqueLock const&);
    mutex_type& mtx;
  };

  template <typename Lk>
  struct SharedLock
  {
    BOOST_CONCEPT_ASSERT(( TimedLock<Lk> ));
    typedef typename Lk::mutex_type mutex_type;

    BOOST_CONCEPT_USAGE(SharedLock)
    {
    }
    SharedLock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    SharedLock operator=(SharedLock const&);
    mutex_type& mtx;

  };

  template <typename Lk>
  struct UpgradeLock
  {
    BOOST_CONCEPT_ASSERT(( SharedLock<Lk> ));
    typedef typename Lk::mutex_type mutex_type;

    BOOST_CONCEPT_USAGE(UpgradeLock)
    {
    }
    UpgradeLock() :
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    UpgradeLock operator=(UpgradeLock const&);
    mutex_type& mtx;
  };

  /**
   * An StrictLock is a scoped lock guard ensuring the mutex is locked on the
   * scope of the lock, by locking the mutex on construction and unlocking it on
   * destruction.
   *
   * Essentially, a StrictLock's role is only to live on the stack as an
   * automatic variable. strict_lock must adhere to a non-copy and non-alias
   * policy. StrictLock disables copying by making the copy constructor and the
   * assignment operator private. While we're at it, let's disable operator new
   * and operator delete; strict locks are not intended to be allocated on the
   * heap. StrictLock avoids aliasing by using a slightly less orthodox and
   * less well-known technique: disable address taking.
   */

  template <typename Lk>
  struct StrictLock
  {
    typedef typename Lk::mutex_type mutex_type;
    BOOST_CONCEPT_ASSERT(( BasicLockable<mutex_type> ));
    BOOST_STATIC_ASSERT(( is_strict_lock<Lk>::value ));

    BOOST_CONCEPT_USAGE( StrictLock)
    {
      if (l1.owns_lock(&mtx)) return;
    }
    StrictLock() :
      l1(*static_cast<Lk*>(0)),
      mtx(*static_cast<mutex_type*>(0))
    {}
  private:
    StrictLock operator=(StrictLock const&);

    Lk const& l1;
    mutex_type const& mtx;

  };

}
#endif
