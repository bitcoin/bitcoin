#ifndef BOOST_THREAD_V2_SHARED_MUTEX_HPP
#define BOOST_THREAD_V2_SHARED_MUTEX_HPP

//  shared_mutex.hpp
//
// Copyright Howard Hinnant 2007-2010.
// Copyright Vicente J. Botet Escriba 2012.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/*
<shared_mutex> synopsis

namespace boost
{
namespace thread_v2
{

class shared_mutex
{
public:

    shared_mutex();
    ~shared_mutex();

    shared_mutex(const shared_mutex&) = delete;
    shared_mutex& operator=(const shared_mutex&) = delete;

    // Exclusive ownership

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    template <class Rep, class Period>
        bool
        try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared();
};

class upgrade_mutex
{
public:

    upgrade_mutex();
    ~upgrade_mutex();

    upgrade_mutex(const upgrade_mutex&) = delete;
    upgrade_mutex& operator=(const upgrade_mutex&) = delete;

    // Exclusive ownership

    void lock();
    bool try_lock();
    template <class Rep, class Period>
        bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock();

    // Shared ownership

    void lock_shared();
    bool try_lock_shared();
    template <class Rep, class Period>
        bool
        try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_shared_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_shared();

    // Upgrade ownership

    void lock_upgrade();
    bool try_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_lock_upgrade_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_lock_upgrade_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade();

    // Shared <-> Exclusive

    bool try_unlock_shared_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_shared();

    // Shared <-> Upgrade

    bool try_unlock_shared_and_lock_upgrade();
    template <class Rep, class Period>
        bool
        try_unlock_shared_and_lock_upgrade_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_shared_and_lock_upgrade_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_upgrade_and_lock_shared();

    // Upgrade <-> Exclusive

    void unlock_upgrade_and_lock();
    bool try_unlock_upgrade_and_lock();
    template <class Rep, class Period>
        bool
        try_unlock_upgrade_and_lock_for(
                            const boost::chrono::duration<Rep, Period>& rel_time);
    template <class Clock, class Duration>
        bool
        try_unlock_upgrade_and_lock_until(
                      const boost::chrono::time_point<Clock, Duration>& abs_time);
    void unlock_and_lock_upgrade();
};

}  // thread_v2
}  // boost

 */

#include <boost/thread/detail/config.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono.hpp>
#endif
#include <climits>
#include <boost/system/system_error.hpp>
#include <boost/bind/bind.hpp>

namespace boost {
  namespace thread_v2 {

    class shared_mutex
    {
      typedef boost::mutex              mutex_t;
      typedef boost::condition_variable cond_t;
      typedef unsigned                  count_t;

      mutex_t mut_;
      cond_t  gate1_;
      // the gate2_ condition variable is only used by functions that
      // have taken write_entered_ but are waiting for no_readers()
      cond_t  gate2_;
      count_t state_;

      static const count_t write_entered_ = 1U << (sizeof(count_t)*CHAR_BIT - 1);
      static const count_t n_readers_ = ~write_entered_;

      bool no_writer() const
      {
        return (state_ & write_entered_) == 0;
      }

      bool one_writer() const
      {
        return (state_ & write_entered_) != 0;
      }

      bool no_writer_no_readers() const
      {
        //return (state_ & write_entered_) == 0 &&
        //       (state_ & n_readers_) == 0;
        return state_ == 0;
      }

      bool no_writer_no_max_readers() const
      {
        return (state_ & write_entered_) == 0 &&
               (state_ & n_readers_) != n_readers_;
      }

      bool no_readers() const
      {
        return (state_ & n_readers_) == 0;
      }

      bool one_or_more_readers() const
      {
        return (state_ & n_readers_) > 0;
      }

      shared_mutex(shared_mutex const&);
      shared_mutex& operator=(shared_mutex const&);

    public:
      shared_mutex();
      ~shared_mutex();

      // Exclusive ownership

      void lock();
      bool try_lock();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#if defined BOOST_THREAD_USES_DATETIME
      template<typename T>
      bool timed_lock(T const & abs_or_rel_time);
#endif
      void unlock();

      // Shared ownership

      void lock_shared();
      bool try_lock_shared();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_shared_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_lock_shared_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#if defined BOOST_THREAD_USES_DATETIME
      template<typename T>
      bool timed_lock_shared(T const & abs_or_rel_time);
#endif
      void unlock_shared();
    };

    inline shared_mutex::shared_mutex()
    : state_(0)
    {
    }

    inline shared_mutex::~shared_mutex()
    {
      boost::lock_guard<mutex_t> _(mut_);
    }

    // Exclusive ownership

    inline void shared_mutex::lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      gate1_.wait(lk, boost::bind(&shared_mutex::no_writer, boost::ref(*this)));
      state_ |= write_entered_;
      gate2_.wait(lk, boost::bind(&shared_mutex::no_readers, boost::ref(*this)));
    }

    inline bool shared_mutex::try_lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!no_writer_no_readers())
      {
        return false;
      }
      state_ = write_entered_;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool shared_mutex::try_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &shared_mutex::no_writer, boost::ref(*this))))
      {
        return false;
      }
      state_ |= write_entered_;
      if (!gate2_.wait_until(lk, abs_time, boost::bind(
            &shared_mutex::no_readers, boost::ref(*this))))
      {
        state_ &= ~write_entered_;
        return false;
      }
      return true;
    }
#endif

#if defined BOOST_THREAD_USES_DATETIME
    template<typename T>
    bool shared_mutex::timed_lock(T const & abs_or_rel_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &shared_mutex::no_writer, boost::ref(*this))))
      {
        return false;
      }
      state_ |= write_entered_;
      if (!gate2_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &shared_mutex::no_readers, boost::ref(*this))))
      {
        state_ &= ~write_entered_;
        return false;
      }
      return true;
    }
#endif

    inline void shared_mutex::unlock()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_writer());
      BOOST_ASSERT(no_readers());
      state_ = 0;
      // notify all since multiple *lock_shared*() calls may be able
      // to proceed in response to this notification
      gate1_.notify_all();
    }

    // Shared ownership

    inline void shared_mutex::lock_shared()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      gate1_.wait(lk, boost::bind(&shared_mutex::no_writer_no_max_readers, boost::ref(*this)));
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
    }

    inline bool shared_mutex::try_lock_shared()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!no_writer_no_max_readers())
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool shared_mutex::try_lock_shared_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &shared_mutex::no_writer_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }
#endif

#if defined BOOST_THREAD_USES_DATETIME
    template<typename T>
    bool shared_mutex::timed_lock_shared(T const & abs_or_rel_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &shared_mutex::no_writer_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }
#endif

    inline void shared_mutex::unlock_shared()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_or_more_readers());
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      if (no_writer())
      {
        if (num_readers == n_readers_ - 1)
          gate1_.notify_one();
      }
      else
      {
        if (num_readers == 0)
          gate2_.notify_one();
      }
    }

  }  // thread_v2
}  // boost

namespace boost {
  namespace thread_v2 {

    class upgrade_mutex
    {
      typedef boost::mutex              mutex_t;
      typedef boost::condition_variable cond_t;
      typedef unsigned                  count_t;

      mutex_t mut_;
      cond_t  gate1_;
      // the gate2_ condition variable is only used by functions that
      // have taken write_entered_ but are waiting for no_readers()
      cond_t  gate2_;
      count_t state_;

      static const unsigned write_entered_ = 1U << (sizeof(count_t)*CHAR_BIT - 1);
      static const unsigned upgradable_entered_ = write_entered_ >> 1;
      static const unsigned n_readers_ = ~(write_entered_ | upgradable_entered_);

      bool no_writer() const
      {
        return (state_ & write_entered_) == 0;
      }

      bool one_writer() const
      {
        return (state_ & write_entered_) != 0;
      }

      bool no_writer_no_max_readers() const
      {
        return (state_ & write_entered_) == 0 &&
               (state_ & n_readers_) != n_readers_;
      }

      bool no_writer_no_upgrader() const
      {
        return (state_ & (write_entered_ | upgradable_entered_)) == 0;
      }

      bool no_writer_no_upgrader_no_readers() const
      {
        //return (state_ & (write_entered_ | upgradable_entered_)) == 0 &&
        //       (state_ & n_readers_) == 0;
        return state_ == 0;
      }

      bool no_writer_no_upgrader_one_reader() const
      {
        //return (state_ & (write_entered_ | upgradable_entered_)) == 0 &&
        //       (state_ & n_readers_) == 1;
        return state_ == 1;
      }

      bool no_writer_no_upgrader_no_max_readers() const
      {
        return (state_ & (write_entered_ | upgradable_entered_)) == 0 &&
               (state_ & n_readers_) != n_readers_;
      }

      bool no_upgrader() const
      {
        return (state_ & upgradable_entered_) == 0;
      }

      bool one_upgrader() const
      {
        return (state_ & upgradable_entered_) != 0;
      }

      bool no_readers() const
      {
        return (state_ & n_readers_) == 0;
      }

      bool one_reader() const
      {
        return (state_ & n_readers_) == 1;
      }

      bool one_or_more_readers() const
      {
        return (state_ & n_readers_) > 0;
      }

      upgrade_mutex(const upgrade_mutex&);
      upgrade_mutex& operator=(const upgrade_mutex&);

    public:
      upgrade_mutex();
      ~upgrade_mutex();

      // Exclusive ownership

      void lock();
      bool try_lock();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_lock_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#if defined BOOST_THREAD_USES_DATETIME
      template<typename T>
      bool timed_lock(T const & abs_or_rel_time);
#endif
      void unlock();

      // Shared ownership

      void lock_shared();
      bool try_lock_shared();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_lock_shared_for(const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_shared_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_lock_shared_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#if defined BOOST_THREAD_USES_DATETIME
      template<typename T>
      bool timed_lock_shared(T const & abs_or_rel_time);
#endif
      void unlock_shared();

      // Upgrade ownership

      void lock_upgrade();
      bool try_lock_upgrade();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_lock_upgrade_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_lock_upgrade_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_lock_upgrade_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#if defined BOOST_THREAD_USES_DATETIME
      template<typename T>
      bool timed_lock_upgrade(T const & abs_or_rel_time);
#endif
      void unlock_upgrade();

      // Shared <-> Exclusive

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
      //bool unlock_shared_and_lock(); // can cause a deadlock if used
      bool try_unlock_shared_and_lock();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_unlock_shared_and_lock_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_shared_and_lock_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_unlock_shared_and_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#endif
      void unlock_and_lock_shared();

      // Shared <-> Upgrade

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
      //bool unlock_shared_and_lock_upgrade(); // can cause a deadlock if used
      bool try_unlock_shared_and_lock_upgrade();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_unlock_shared_and_lock_upgrade_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_shared_and_lock_upgrade_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_unlock_shared_and_lock_upgrade_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
#endif
      void unlock_upgrade_and_lock_shared();

      // Upgrade <-> Exclusive

      void unlock_upgrade_and_lock();
      bool try_unlock_upgrade_and_lock();
#ifdef BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      bool try_unlock_upgrade_and_lock_for(
          const boost::chrono::duration<Rep, Period>& rel_time)
      {
        return try_unlock_upgrade_and_lock_until(chrono::steady_clock::now() + rel_time);
      }
      template <class Clock, class Duration>
      bool try_unlock_upgrade_and_lock_until(
          const boost::chrono::time_point<Clock, Duration>& abs_time);
#endif
      void unlock_and_lock_upgrade();
    };

    inline upgrade_mutex::upgrade_mutex()
    : gate1_(),
      gate2_(),
      state_(0)
    {
    }

    inline upgrade_mutex::~upgrade_mutex()
    {
      boost::lock_guard<mutex_t> _(mut_);
    }

    // Exclusive ownership

    inline void upgrade_mutex::lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      gate1_.wait(lk, boost::bind(&upgrade_mutex::no_writer_no_upgrader, boost::ref(*this)));
      state_ |= write_entered_;
      gate2_.wait(lk, boost::bind(&upgrade_mutex::no_readers, boost::ref(*this)));
    }

    inline bool upgrade_mutex::try_lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!no_writer_no_upgrader_no_readers())
      {
        return false;
      }
      state_ = write_entered_;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader, boost::ref(*this))))
      {
        return false;
      }
      state_ |= write_entered_;
      if (!gate2_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_readers, boost::ref(*this))))
      {
        state_ &= ~write_entered_;
        return false;
      }
      return true;
    }
#endif

#if defined BOOST_THREAD_USES_DATETIME
    template<typename T>
    bool upgrade_mutex::timed_lock(T const & abs_or_rel_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader, boost::ref(*this))))
      {
        return false;
      }
      state_ |= write_entered_;
      if (!gate2_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &upgrade_mutex::no_readers, boost::ref(*this))))
      {
        state_ &= ~write_entered_;
        return false;
      }
      return true;
    }
#endif

    inline void upgrade_mutex::unlock()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_writer());
      BOOST_ASSERT(no_upgrader());
      BOOST_ASSERT(no_readers());
      state_ = 0;
      // notify all since multiple *lock_shared*() calls and a *lock_upgrade*()
      // call may be able to proceed in response to this notification
      gate1_.notify_all();
    }

    // Shared ownership

    inline void upgrade_mutex::lock_shared()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      gate1_.wait(lk, boost::bind(&upgrade_mutex::no_writer_no_max_readers, boost::ref(*this)));
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
    }

    inline bool upgrade_mutex::try_lock_shared()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!no_writer_no_max_readers())
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_lock_shared_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_writer_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }
#endif

#if defined BOOST_THREAD_USES_DATETIME
    template<typename T>
    bool upgrade_mutex::timed_lock_shared(T const & abs_or_rel_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &upgrade_mutex::no_writer_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      return true;
    }
#endif

    inline void upgrade_mutex::unlock_shared()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_or_more_readers());
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~n_readers_;
      state_ |= num_readers;
      if (no_writer())
      {
        if (num_readers == n_readers_ - 1)
          gate1_.notify_one();
      }
      else
      {
        if (num_readers == 0)
          gate2_.notify_one();
      }
    }

    // Upgrade ownership

    inline void upgrade_mutex::lock_upgrade()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      gate1_.wait(lk, boost::bind(&upgrade_mutex::no_writer_no_upgrader_no_max_readers, boost::ref(*this)));
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= upgradable_entered_ | num_readers;
    }

    inline bool upgrade_mutex::try_lock_upgrade()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!no_writer_no_upgrader_no_max_readers())
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= upgradable_entered_ | num_readers;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_lock_upgrade_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= upgradable_entered_ | num_readers;
      return true;
    }
#endif

#if defined BOOST_THREAD_USES_DATETIME
    template<typename T>
    bool upgrade_mutex::timed_lock_upgrade(T const & abs_or_rel_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      if (!gate1_.timed_wait(lk, abs_or_rel_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader_no_max_readers, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) + 1;
      state_ &= ~n_readers_;
      state_ |= upgradable_entered_ | num_readers;
      return true;
    }
#endif

    inline void upgrade_mutex::unlock_upgrade()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(no_writer());
      BOOST_ASSERT(one_upgrader());
      BOOST_ASSERT(one_or_more_readers());
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~(upgradable_entered_ | n_readers_);
      state_ |= num_readers;
      // notify all since both a *lock*() and a *lock_shared*() call
      // may be able to proceed in response to this notification
      gate1_.notify_all();
    }

    // Shared <-> Exclusive

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
    inline bool upgrade_mutex::try_unlock_shared_and_lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(one_or_more_readers());
      if (!no_writer_no_upgrader_one_reader())
      {
        return false;
      }
      state_ = write_entered_;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_unlock_shared_and_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(one_or_more_readers());
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader, boost::ref(*this))))
      {
        return false;
      }
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~n_readers_;
      state_ |= (write_entered_ | num_readers);
      if (!gate2_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_readers, boost::ref(*this))))
      {
        ++num_readers;
        state_ &= ~(write_entered_ | n_readers_);
        state_ |= num_readers;
        return false;
      }
      return true;
    }
#endif
#endif

    inline void upgrade_mutex::unlock_and_lock_shared()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_writer());
      BOOST_ASSERT(no_upgrader());
      BOOST_ASSERT(no_readers());
      state_ = 1;
      // notify all since multiple *lock_shared*() calls and a *lock_upgrade*()
      // call may be able to proceed in response to this notification
      gate1_.notify_all();
    }

    // Shared <-> Upgrade

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
    inline bool upgrade_mutex::try_unlock_shared_and_lock_upgrade()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(one_or_more_readers());
      if (!no_writer_no_upgrader())
      {
        return false;
      }
      state_ |= upgradable_entered_;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_unlock_shared_and_lock_upgrade_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(one_or_more_readers());
      if (!gate1_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_writer_no_upgrader, boost::ref(*this))))
      {
        return false;
      }
      state_ |= upgradable_entered_;
      return true;
    }
#endif
#endif

    inline void upgrade_mutex::unlock_upgrade_and_lock_shared()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(no_writer());
      BOOST_ASSERT(one_upgrader());
      BOOST_ASSERT(one_or_more_readers());
      state_ &= ~upgradable_entered_;
      // notify all since only one *lock*() or *lock_upgrade*() call can win and
      // proceed in response to this notification, but a *lock_shared*() call may
      // also be waiting and could steal the notification
      gate1_.notify_all();
    }

    // Upgrade <-> Exclusive

    inline void upgrade_mutex::unlock_upgrade_and_lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(no_writer());
      BOOST_ASSERT(one_upgrader());
      BOOST_ASSERT(one_or_more_readers());
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~(upgradable_entered_ | n_readers_);
      state_ |= write_entered_ | num_readers;
      gate2_.wait(lk, boost::bind(&upgrade_mutex::no_readers, boost::ref(*this)));
    }

    inline bool upgrade_mutex::try_unlock_upgrade_and_lock()
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(no_writer());
      BOOST_ASSERT(one_upgrader());
      BOOST_ASSERT(one_or_more_readers());
      if (!one_reader())
      {
        return false;
      }
      state_ = write_entered_;
      return true;
    }

#ifdef BOOST_THREAD_USES_CHRONO
    template <class Clock, class Duration>
    bool upgrade_mutex::try_unlock_upgrade_and_lock_until(
        const boost::chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<mutex_t> lk(mut_);
      BOOST_ASSERT(no_writer());
      BOOST_ASSERT(one_upgrader());
      BOOST_ASSERT(one_or_more_readers());
      count_t num_readers = (state_ & n_readers_) - 1;
      state_ &= ~(upgradable_entered_ | n_readers_);
      state_ |= (write_entered_ | num_readers);
      if (!gate2_.wait_until(lk, abs_time, boost::bind(
            &upgrade_mutex::no_readers, boost::ref(*this))))
      {
        ++num_readers;
        state_ &= ~(write_entered_ | n_readers_);
        state_ |= (upgradable_entered_ | num_readers);
        return false;
      }
      return true;
    }
#endif

    inline void upgrade_mutex::unlock_and_lock_upgrade()
    {
      boost::lock_guard<mutex_t> _(mut_);
      BOOST_ASSERT(one_writer());
      BOOST_ASSERT(no_upgrader());
      BOOST_ASSERT(no_readers());
      state_ = upgradable_entered_ | 1;
      // notify all since multiple *lock_shared*() calls may be able
      // to proceed in response to this notification
      gate1_.notify_all();
    }

  }  // thread_v2
}  // boost

namespace boost {
  //using thread_v2::shared_mutex;
  using thread_v2::upgrade_mutex;
  typedef thread_v2::upgrade_mutex shared_mutex;
}

#endif
