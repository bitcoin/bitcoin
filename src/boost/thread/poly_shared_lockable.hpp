//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_POLY_SHARED_LOCKABLE_HPP
#define BOOST_THREAD_POLY_SHARED_LOCKABLE_HPP

#include <boost/thread/poly_lockable.hpp>
#include <boost/chrono/chrono.hpp>

namespace boost
{


  //[shared_poly_lockable
  class shared_poly_lockable: public timed_poly_lockable
  {
  public:
    virtual ~shared_poly_lockable() = 0;

    virtual void lock_shared() = 0;
    virtual bool try_lock_shared() = 0;
    virtual void unlock_shared() = 0;

    virtual bool try_lock_shared_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_lock_shared_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_lock_shared_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_lock_shared_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_lock_shared_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_lock_shared_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_lock_shared_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

  };
  //]

  // A proper name for shared_poly_lockable, consistent with naming scheme of other polymorphic wrappers
  typedef shared_poly_lockable poly_shared_lockable;

  //[upgrade_poly_lockable
  class upgrade_poly_lockable: public shared_poly_lockable
  {
  public:
    virtual ~upgrade_poly_lockable() = 0;

    virtual void lock_upgrade() = 0;
    virtual bool try_lock_upgrade() = 0;
    virtual void unlock_upgrade() = 0;

    virtual bool try_lock_upgrade_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_lock_upgrade_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_lock_upgrade_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_lock_upgrade_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_lock_upgrade_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_lock_upgrade_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_lock_upgrade_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

    virtual bool try_unlock_shared_and_lock() = 0;

    virtual bool try_unlock_shared_and_lock_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_unlock_shared_and_lock_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_unlock_shared_and_lock_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_unlock_shared_and_lock_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_unlock_shared_and_lock_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

    virtual void unlock_and_lock_shared() = 0;
    virtual bool try_unlock_shared_and_lock_upgrade() = 0;

    virtual bool try_unlock_shared_and_lock_upgrade_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_unlock_shared_and_lock_upgrade_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_upgrade_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_unlock_shared_and_lock_upgrade_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_unlock_shared_and_lock_upgrade_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_upgrade_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_unlock_shared_and_lock_upgrade_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

    virtual void unlock_and_lock_upgrade() = 0;
    virtual void unlock_upgrade_and_lock() = 0;
    virtual bool try_unlock_upgrade_and_lock() = 0;

    virtual bool try_unlock_upgrade_and_lock_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_unlock_upgrade_and_lock_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_unlock_upgrade_and_lock_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_unlock_upgrade_and_lock_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_unlock_upgrade_and_lock_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_unlock_upgrade_and_lock_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_unlock_upgrade_and_lock_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

    virtual void unlock_upgrade_and_lock_shared() = 0;

  };
  //]

  // A proper name for upgrade_poly_lockable, consistent with naming scheme of other polymorphic wrappers
  typedef upgrade_poly_lockable poly_upgrade_lockable;
}
#endif
