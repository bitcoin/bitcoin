//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_NULL_MUTEX_HPP
#define BOOST_THREAD_NULL_MUTEX_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/chrono/chrono.hpp>

/// \file
/// Describes null_mutex class

namespace boost
{

  /// Implements a mutex that simulates a mutex without doing any operation and
  /// simulates a successful operation.
  class null_mutex
  {
  public:

    BOOST_THREAD_NO_COPYABLE( null_mutex) /*< no copyable >*/

    null_mutex() {}

    /// Simulates a mutex lock() operation. Empty function.
    void lock()
    {
    }

    /// Simulates a mutex try_lock() operation.
    /// Equivalent to "return true;"
    bool try_lock()
    {
      return true;
    }

    /// Simulates a mutex unlock() operation.
    /// Empty function.
    void unlock()
    {
    }

#ifdef BOOST_THREAD_USES_CHRONO
    /// Simulates a mutex try_lock_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_lock_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }

    /// Simulates a mutex try_lock_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_lock_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }
#endif

    /// Simulates a mutex lock_shared() operation.
    /// Empty function.
    void lock_shared()
    {
    }

    /// Simulates a mutex try_lock_shared() operation.
    /// Equivalent to "return true;"
    bool try_lock_shared()
    {
      return true;
    }

    /// Simulates a mutex unlock_shared() operation.
    /// Empty function.
    void unlock_shared()
    {
    }

    /// Simulates a mutex try_lock_shared_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_lock_shared_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }
    /// Simulates a mutex try_lock_shared_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_lock_shared_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }

    /// Simulates a mutex lock_upgrade() operation.
    /// Empty function.
    void lock_upgrade()
    {
    }

    /// Simulates a mutex try_lock_upgrade() operation.
    /// Equivalent to "return true;"
    bool try_lock_upgrade()
    {
      return true;
    }

    /// Simulates a mutex unlock_upgrade() operation.
    /// Empty function.
    void unlock_upgrade()
    {
    }

    /// Simulates a mutex try_lock_upgrade_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_lock_upgrade_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }

    /// Simulates a mutex try_lock_upgrade_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_lock_upgrade_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }

    /// Simulates a mutex try_unlock_shared_and_lock() operation.
    /// Equivalent to "return true;"
    bool try_unlock_shared_and_lock()
    {
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    /// Simulates a mutex try_unlock_shared_and_lock_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }

    /// Simulates a mutex try_unlock_shared_and_lock_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }
#endif

    /// Simulates unlock_and_lock_shared().
    /// Empty function.
    void unlock_and_lock_shared()
    {
    }

    /// Simulates a mutex try_unlock_shared_and_lock_upgrade() operation.
    /// Equivalent to "return true;"
    bool try_unlock_shared_and_lock_upgrade()
    {
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    /// Simulates a mutex try_unlock_shared_and_lock_upgrade_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_unlock_shared_and_lock_upgrade_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }

    /// Simulates a mutex try_unlock_shared_and_lock_upgrade_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_unlock_shared_and_lock_upgrade_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }
#endif

    /// Simulates unlock_and_lock_upgrade().
    /// Empty function.
    void unlock_and_lock_upgrade()
    {
    }

    /// Simulates unlock_upgrade_and_lock().
    /// Empty function.
    void unlock_upgrade_and_lock()
    {
    }

    /// Simulates a mutex try_unlock_upgrade_and_lock() operation.
    /// Equivalent to "return true;"
    bool try_unlock_upgrade_and_lock()
    {
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    /// Simulates a mutex try_unlock_upgrade_and_lock_until() operation.
    /// Equivalent to "return true;"
    template <typename Clock, typename Duration>
    bool try_unlock_upgrade_and_lock_until(chrono::time_point<Clock, Duration> const &)
    {
      return true;
    }

    /// Simulates a mutex try_unlock_upgrade_and_lock_for() operation.
    /// Equivalent to "return true;"
    template <typename Rep, typename Period>
    bool try_unlock_upgrade_and_lock_for(chrono::duration<Rep, Period> const &)
    {
      return true;
    }
#endif

    /// Simulates unlock_upgrade_and_lock_shared().
    /// Empty function.
    void unlock_upgrade_and_lock_shared()
    {
    }

  };

} //namespace boost {


#endif
