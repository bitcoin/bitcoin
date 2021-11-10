//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2008-2009,2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_THREAD_POLY_LOCKABLE_HPP
#define BOOST_THREAD_POLY_LOCKABLE_HPP

#include <boost/thread/detail/delete.hpp>
#include <boost/chrono/chrono.hpp>

namespace boost
{

  //[basic_poly_lockable
  class basic_poly_lockable
  {
  public:

    virtual ~basic_poly_lockable() = 0;

    virtual void lock() = 0;
    virtual void unlock() = 0;

  };
  //]

  // A proper name for basic_poly_lockable, consistent with naming scheme of other polymorphic wrappers
  typedef basic_poly_lockable poly_basic_lockable;

  //[poly_lockable
  class poly_lockable : public basic_poly_lockable
  {
  public:

    virtual ~poly_lockable() = 0;
    virtual bool try_lock() = 0;
  };
  //]

  //[timed_poly_lockable
  class timed_poly_lockable: public poly_lockable
  {
  public:
    virtual ~timed_poly_lockable()=0;

    virtual bool try_lock_until(chrono::system_clock::time_point const & abs_time)=0;
    virtual bool try_lock_until(chrono::steady_clock::time_point const & abs_time)=0;
    template <typename Clock, typename Duration>
    bool try_lock_until(chrono::time_point<Clock, Duration> const & abs_time)
    {
      return try_lock_until(chrono::time_point_cast<Clock::time_point>(abs_time));
    }

    virtual bool try_lock_for(chrono::nanoseconds const & relative_time)=0;
    template <typename Rep, typename Period>
    bool try_lock_for(chrono::duration<Rep, Period> const & rel_time)
    {
      return try_lock_for(chrono::duration_cast<chrono::nanoseconds>(rel_time));
    }

  };
  //]

  // A proper name for timed_poly_lockable, consistent with naming scheme of other polymorphic wrappers
  typedef timed_poly_lockable poly_timed_lockable;
}
#endif
