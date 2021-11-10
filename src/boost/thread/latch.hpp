// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2013 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_LATCH_HPP
#define BOOST_THREAD_LATCH_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/counter.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/assert.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  class latch
  {
    /// @Requires: count_ must be greater than 0
    /// Effect: Decrement the count. Unlocks the lock and notify anyone waiting if we reached zero.
    /// Returns: true if count_ reached the value 0.
    /// @ThreadSafe ensured by the @c lk parameter
    bool count_down(unique_lock<mutex> &)
    /// pre_condition (count_ > 0)
    {
      BOOST_ASSERT(count_ > 0);
      if (--count_ == 0)
      {
        ++generation_;
        //lk.unlock();
        cond_.notify_all();
        return true;
      }
      return false;
    }
    /// Effect: Decrement the count is > 0. Unlocks the lock notify anyone waiting if we reached zero.
    /// Returns: true if count_ is 0.
    /// @ThreadSafe ensured by the @c lk parameter
    bool try_count_down(unique_lock<mutex> &lk)
    {
      if (count_ > 0)
      {
        return count_down(lk);
      }
      return true;
    }
  public:
    BOOST_THREAD_NO_COPYABLE( latch)

    /// Constructs a latch with a given count.
    latch(std::size_t count) :
      count_(count), generation_(0)
    {
    }

    /// Destructor
    /// Precondition: No threads are waiting or invoking count_down on @c *this.

    ~latch()
    {

    }

    /// Blocks until the latch has counted down to zero.
    void wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_ == 0) return;
      std::size_t generation(generation_);
      cond_.wait(lk, detail::not_equal(generation, generation_));
    }

    /// @return true if the internal counter is already 0, false otherwise
    bool try_wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      return (count_ == 0);
    }

    /// try to wait for a specified amount of time is elapsed.
    /// @return whether there is a timeout or not.
    template <class Rep, class Period>
    cv_status wait_for(const chrono::duration<Rep, Period>& rel_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_ == 0) return cv_status::no_timeout;
      std::size_t generation(generation_);
      return cond_.wait_for(lk, rel_time, detail::not_equal(generation, generation_))
              ? cv_status::no_timeout
              : cv_status::timeout;
    }

    /// try to wait until the specified time_point is reached
    /// @return whether there were a timeout or not.
    template <class Clock, class Duration>
    cv_status wait_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      if (count_ == 0) return cv_status::no_timeout;
      std::size_t generation(generation_);
      return cond_.wait_until(lk, abs_time, detail::not_equal(generation, generation_))
          ? cv_status::no_timeout
          : cv_status::timeout;
    }

    /// Decrement the count and notify anyone waiting if we reach zero.
    /// @Requires count must be greater than 0
    void count_down()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      count_down(lk);
    }
    /// Effect: Decrement the count if it is > 0 and notify anyone waiting if we reached zero.
    /// Returns: true if count_ was 0 or reached 0.
    bool try_count_down()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      return try_count_down(lk);
    }
    void signal()
    {
      count_down();
    }

    /// Decrement the count and notify anyone waiting if we reach zero.
    /// Blocks until the latch has counted down to zero.
    /// @Requires count must be greater than 0
    void count_down_and_wait()
    {
      boost::unique_lock<boost::mutex> lk(mutex_);
      std::size_t generation(generation_);
      if (count_down(lk))
      {
        return;
      }
      cond_.wait(lk, detail::not_equal(generation, generation_));
    }
    void sync()
    {
      count_down_and_wait();
    }

    /// Reset the counter
    /// #Requires This method may only be invoked when there are no other threads currently inside the count_down_and_wait() method.
    void reset(std::size_t count)
    {
      boost::lock_guard<boost::mutex> lk(mutex_);
      //BOOST_ASSERT(count_ == 0);
      count_ = count;
    }

  private:
    mutex mutex_;
    condition_variable cond_;
    std::size_t count_;
    std::size_t generation_;
  };

} // namespace boost

#include <boost/config/abi_suffix.hpp>

#endif
