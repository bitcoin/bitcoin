//
// detail/chrono_time_traits.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_DETAIL_CHRONO_TIME_TRAITS_HPP
#define BOOST_ASIO_DETAIL_CHRONO_TIME_TRAITS_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/cstdint.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace detail {

// Helper template to compute the greatest common divisor.
template <int64_t v1, int64_t v2>
struct gcd { enum { value = gcd<v2, v1 % v2>::value }; };

template <int64_t v1>
struct gcd<v1, 0> { enum { value = v1 }; };

// Adapts std::chrono clocks for use with a deadline timer.
template <typename Clock, typename WaitTraits>
struct chrono_time_traits
{
  // The clock type.
  typedef Clock clock_type;

  // The duration type of the clock.
  typedef typename clock_type::duration duration_type;

  // The time point type of the clock.
  typedef typename clock_type::time_point time_type;

  // The period of the clock.
  typedef typename duration_type::period period_type;

  // Get the current time.
  static time_type now()
  {
    return clock_type::now();
  }

  // Add a duration to a time.
  static time_type add(const time_type& t, const duration_type& d)
  {
    const time_type epoch;
    if (t >= epoch)
    {
      if ((time_type::max)() - t < d)
        return (time_type::max)();
    }
    else // t < epoch
    {
      if (-(t - (time_type::min)()) > d)
        return (time_type::min)();
    }

    return t + d;
  }

  // Subtract one time from another.
  static duration_type subtract(const time_type& t1, const time_type& t2)
  {
    const time_type epoch;
    if (t1 >= epoch)
    {
      if (t2 >= epoch)
      {
        return t1 - t2;
      }
      else if (t2 == (time_type::min)())
      {
        return (duration_type::max)();
      }
      else if ((time_type::max)() - t1 < epoch - t2)
      {
        return (duration_type::max)();
      }
      else
      {
        return t1 - t2;
      }
    }
    else // t1 < epoch
    {
      if (t2 < epoch)
      {
        return t1 - t2;
      }
      else if (t1 == (time_type::min)())
      {
        return (duration_type::min)();
      }
      else if ((time_type::max)() - t2 < epoch - t1)
      {
        return (duration_type::min)();
      }
      else
      {
        return -(t2 - t1);
      }
    }
  }

  // Test whether one time is less than another.
  static bool less_than(const time_type& t1, const time_type& t2)
  {
    return t1 < t2;
  }

  // Implement just enough of the posix_time::time_duration interface to supply
  // what the timer_queue requires.
  class posix_time_duration
  {
  public:
    explicit posix_time_duration(const duration_type& d)
      : d_(d)
    {
    }

    int64_t ticks() const
    {
      return d_.count();
    }

    int64_t total_seconds() const
    {
      return duration_cast<1, 1>();
    }

    int64_t total_milliseconds() const
    {
      return duration_cast<1, 1000>();
    }

    int64_t total_microseconds() const
    {
      return duration_cast<1, 1000000>();
    }

  private:
    template <int64_t Num, int64_t Den>
    int64_t duration_cast() const
    {
      const int64_t num1 = period_type::num / gcd<period_type::num, Num>::value;
      const int64_t num2 = Num / gcd<period_type::num, Num>::value;

      const int64_t den1 = period_type::den / gcd<period_type::den, Den>::value;
      const int64_t den2 = Den / gcd<period_type::den, Den>::value;

      const int64_t num = num1 * den2;
      const int64_t den = num2 * den1;

      if (num == 1 && den == 1)
        return ticks();
      else if (num != 1 && den == 1)
        return ticks() * num;
      else if (num == 1 && period_type::den != 1)
        return ticks() / den;
      else
        return ticks() * num / den;
    }

    duration_type d_;
  };

  // Convert to POSIX duration type.
  static posix_time_duration to_posix_duration(const duration_type& d)
  {
    return posix_time_duration(WaitTraits::to_wait_duration(d));
  }
};

} // namespace detail
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_DETAIL_CHRONO_TIME_TRAITS_HPP
