#ifndef BOOST_THREAD_DETAIL_PLATFORM_TIME_HPP
#define BOOST_THREAD_DETAIL_PLATFORM_TIME_HPP
//  (C) Copyright 2007-8 Anthony Williams
//  (C) Copyright 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread/detail/config.hpp>
#include <boost/thread/thread_time.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/date_time/posix_time/conversion.hpp>
#endif
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif

#if defined(BOOST_THREAD_CHRONO_WINDOWS_API)
#include <boost/winapi/time.hpp>
#include <boost/winapi/timers.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#elif defined(BOOST_THREAD_CHRONO_MAC_API)
#include <sys/time.h> //for gettimeofday and timeval
#include <mach/mach_time.h>  // mach_absolute_time, mach_timebase_info_data_t

#else
#include <time.h>  // for clock_gettime
#endif

#include <limits>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
//typedef boost::int_least64_t time_max_t;
typedef boost::intmax_t time_max_t;

#if defined BOOST_THREAD_CHRONO_MAC_API
namespace threads
{

namespace chrono_details
{

// steady_clock

// Note, in this implementation steady_clock and high_resolution_clock
//   are the same clock.  They are both based on mach_absolute_time().
//   mach_absolute_time() * MachInfo.numer / MachInfo.denom is the number of
//   nanoseconds since the computer booted up.  MachInfo.numer and MachInfo.denom
//   are run time constants supplied by the OS.  This clock has no relationship
//   to the Gregorian calendar.  It's main use is as a high resolution timer.

// MachInfo.numer / MachInfo.denom is often 1 on the latest equipment.  Specialize
//   for that case as an optimization.

inline time_max_t
steady_simplified()
{
    return mach_absolute_time();
}

inline double compute_steady_factor(kern_return_t& err)
{
    mach_timebase_info_data_t MachInfo;
    err = mach_timebase_info(&MachInfo);
    if ( err != 0  ) {
        return 0;
    }
    return static_cast<double>(MachInfo.numer) / MachInfo.denom;
}

inline time_max_t steady_full()
{
    kern_return_t err;
    const double factor = chrono_details::compute_steady_factor(err);
    if (err != 0)
    {
      BOOST_ASSERT(0 && "Boost::Chrono - Internal Error");
    }
    return static_cast<time_max_t>(mach_absolute_time() * factor);
}


typedef time_max_t (*FP)();

inline FP init_steady_clock(kern_return_t & err)
{
    mach_timebase_info_data_t MachInfo;
    err = mach_timebase_info(&MachInfo);
    if ( err != 0  )
    {
        return 0;
    }

    if (MachInfo.numer == MachInfo.denom)
    {
        return &chrono_details::steady_simplified;
    }
    return &chrono_details::steady_full;
}

}
}
#endif

  namespace detail
  {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
    inline timespec ns_to_timespec(boost::time_max_t const& ns)
    {
      boost::time_max_t s = ns / 1000000000l;
      timespec ts;
      ts.tv_sec = static_cast<long> (s);
      ts.tv_nsec = static_cast<long> (ns - s * 1000000000l);
      return ts;
    }
    inline boost::time_max_t timespec_to_ns(timespec const& ts)
    {
      return static_cast<boost::time_max_t>(ts.tv_sec) * 1000000000l + ts.tv_nsec;
    }
#endif

    struct platform_duration
    {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
      explicit platform_duration(timespec const& v) : ts_val(v) {}
      timespec const& getTs() const { return ts_val; }

      explicit platform_duration(boost::time_max_t const& ns = 0) : ts_val(ns_to_timespec(ns)) {}
      boost::time_max_t getNs() const { return timespec_to_ns(ts_val); }
#else
      explicit platform_duration(boost::time_max_t const& ns = 0) : ns_val(ns) {}
      boost::time_max_t getNs() const { return ns_val; }
#endif

#if defined BOOST_THREAD_USES_DATETIME
      platform_duration(boost::posix_time::time_duration const& rel_time)
      {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
        ts_val.tv_sec = rel_time.total_seconds();
        ts_val.tv_nsec = static_cast<long>(rel_time.fractional_seconds() * (1000000000l / rel_time.ticks_per_second()));
#else
        ns_val = static_cast<boost::time_max_t>(rel_time.total_seconds()) * 1000000000l;
        ns_val += rel_time.fractional_seconds() * (1000000000l / rel_time.ticks_per_second());
#endif
      }
#endif

#if defined BOOST_THREAD_USES_CHRONO
      template <class Rep, class Period>
      platform_duration(chrono::duration<Rep, Period> const& d)
      {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
        ts_val = ns_to_timespec(chrono::ceil<chrono::nanoseconds>(d).count());
#else
        ns_val = chrono::ceil<chrono::nanoseconds>(d).count();
#endif
      }
#endif

      boost::time_max_t getMs() const
      {
        const boost::time_max_t ns = getNs();
        // ceil/floor away from zero
        if (ns >= 0)
        {
          // return ceiling of positive numbers
          return (ns + 999999) / 1000000;
        }
        else
        {
          // return floor of negative numbers
          return (ns - 999999) / 1000000;
        }
      }

      static platform_duration zero()
      {
        return platform_duration(0);
      }

    private:
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
      timespec ts_val;
#else
      boost::time_max_t ns_val;
#endif
    };

    inline bool operator==(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() == rhs.getNs();
    }
    inline bool operator!=(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() != rhs.getNs();
    }
    inline bool operator<(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() < rhs.getNs();
    }
    inline bool operator<=(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() <= rhs.getNs();
    }
    inline bool operator>(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() > rhs.getNs();
    }
    inline bool operator>=(platform_duration const& lhs, platform_duration const& rhs)
    {
      return lhs.getNs() >= rhs.getNs();
    }

    static inline platform_duration platform_milliseconds(long const& ms)
    {
      return platform_duration(ms * 1000000l);
    }

    struct real_platform_timepoint
    {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
      explicit real_platform_timepoint(timespec const& v) : dur(v) {}
      timespec const& getTs() const { return dur.getTs(); }
#endif

      explicit real_platform_timepoint(boost::time_max_t const& ns) : dur(ns) {}
      boost::time_max_t getNs() const { return dur.getNs(); }

#if defined BOOST_THREAD_USES_DATETIME
      real_platform_timepoint(boost::system_time const& abs_time)
        : dur(abs_time - boost::posix_time::from_time_t(0)) {}
#endif

#if defined BOOST_THREAD_USES_CHRONO
      template <class Duration>
      real_platform_timepoint(chrono::time_point<chrono::system_clock, Duration> const& abs_time)
        : dur(abs_time.time_since_epoch()) {}
#endif

    private:
      platform_duration dur;
    };

    inline bool operator==(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() == rhs.getNs();
    }
    inline bool operator!=(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() != rhs.getNs();
    }
    inline bool operator<(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() < rhs.getNs();
    }
    inline bool operator<=(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() <= rhs.getNs();
    }
    inline bool operator>(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() > rhs.getNs();
    }
    inline bool operator>=(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return lhs.getNs() >= rhs.getNs();
    }

    inline real_platform_timepoint operator+(real_platform_timepoint const& lhs, platform_duration const& rhs)
    {
      return real_platform_timepoint(lhs.getNs() + rhs.getNs());
    }
    inline real_platform_timepoint operator+(platform_duration const& lhs, real_platform_timepoint const& rhs)
    {
      return real_platform_timepoint(lhs.getNs() + rhs.getNs());
    }
    inline platform_duration operator-(real_platform_timepoint const& lhs, real_platform_timepoint const& rhs)
    {
      return platform_duration(lhs.getNs() - rhs.getNs());
    }

    struct real_platform_clock
    {
      static real_platform_timepoint now()
      {
#if defined(BOOST_THREAD_CHRONO_WINDOWS_API)
        boost::winapi::FILETIME_ ft;
        boost::winapi::GetSystemTimeAsFileTime(&ft);  // never fails
        boost::time_max_t ns = ((((static_cast<boost::time_max_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime) - 116444736000000000LL) * 100LL);
        return real_platform_timepoint(ns);
#elif defined(BOOST_THREAD_CHRONO_MAC_API)
        timeval tv;
        ::gettimeofday(&tv, 0);
        timespec ts;
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
        return real_platform_timepoint(ts);
#else
        timespec ts;
        if ( ::clock_gettime( CLOCK_REALTIME, &ts ) )
        {
          BOOST_ASSERT(0 && "Boost::Thread - clock_gettime(CLOCK_REALTIME) Internal Error");
          return real_platform_timepoint(0);
        }
        return real_platform_timepoint(ts);
#endif
      }
    };

#if defined(BOOST_THREAD_HAS_MONO_CLOCK)

  struct mono_platform_timepoint
  {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API

    explicit mono_platform_timepoint(timespec const& v) : dur(v) {}
    timespec const& getTs() const { return dur.getTs(); }
#endif

    explicit mono_platform_timepoint(boost::time_max_t const& ns) : dur(ns) {}
    boost::time_max_t getNs() const { return dur.getNs(); }

#if defined BOOST_THREAD_USES_CHRONO
    // This conversion assumes that chrono::steady_clock::time_point and mono_platform_timepoint share the same epoch.
    template <class Duration>
    mono_platform_timepoint(chrono::time_point<chrono::steady_clock, Duration> const& abs_time)
      : dur(abs_time.time_since_epoch()) {}
#endif

    // can't name this max() since that is a macro on some Windows systems
    static mono_platform_timepoint getMax()
    {
#if defined BOOST_THREAD_CHRONO_POSIX_API || defined BOOST_THREAD_CHRONO_MAC_API
      timespec ts;
      ts.tv_sec = (std::numeric_limits<time_t>::max)();
      ts.tv_nsec = 999999999;
      return mono_platform_timepoint(ts);
#else
      boost::time_max_t ns = (std::numeric_limits<boost::time_max_t>::max)();
      return mono_platform_timepoint(ns);
#endif
    }

  private:
    platform_duration dur;
  };

  inline bool operator==(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() == rhs.getNs();
  }
  inline bool operator!=(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() != rhs.getNs();
  }
  inline bool operator<(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() < rhs.getNs();
  }
  inline bool operator<=(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() <= rhs.getNs();
  }
  inline bool operator>(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() > rhs.getNs();
  }
  inline bool operator>=(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return lhs.getNs() >= rhs.getNs();
  }

  inline mono_platform_timepoint operator+(mono_platform_timepoint const& lhs, platform_duration const& rhs)
  {
    return mono_platform_timepoint(lhs.getNs() + rhs.getNs());
  }
  inline mono_platform_timepoint operator+(platform_duration const& lhs, mono_platform_timepoint const& rhs)
  {
    return mono_platform_timepoint(lhs.getNs() + rhs.getNs());
  }
  inline platform_duration operator-(mono_platform_timepoint const& lhs, mono_platform_timepoint const& rhs)
  {
    return platform_duration(lhs.getNs() - rhs.getNs());
  }

  struct mono_platform_clock
  {
    static mono_platform_timepoint now()
    {
#if defined(BOOST_THREAD_CHRONO_WINDOWS_API)
#if defined(BOOST_THREAD_USES_CHRONO)
      // Use QueryPerformanceCounter() to match the implementation in Boost
      // Chrono so that chrono::steady_clock::now() and this function share the
      // same epoch and so can be converted between each other.
      boost::winapi::LARGE_INTEGER_ freq;
      if ( !boost::winapi::QueryPerformanceFrequency( &freq ) )
      {
        BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceFrequency Internal Error");
        return mono_platform_timepoint(0);
      }
      if ( freq.QuadPart <= 0 )
      {
        BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceFrequency Internal Error");
        return mono_platform_timepoint(0);
      }

      boost::winapi::LARGE_INTEGER_ pcount;
      unsigned times=0;
      while ( ! boost::winapi::QueryPerformanceCounter( &pcount ) )
      {
        if ( ++times > 3 )
        {
          BOOST_ASSERT(0 && "Boost::Thread - QueryPerformanceCounter Internal Error");
          return mono_platform_timepoint(0);
        }
      }

      long double ns = 1000000000.0L * pcount.QuadPart / freq.QuadPart;
      return mono_platform_timepoint(static_cast<boost::time_max_t>(ns));
#else
      // Use GetTickCount64() because it's more reliable on older
      // systems like Windows XP and Windows Server 2003.
      win32::ticks_type msec = win32::gettickcount64();
      return mono_platform_timepoint(msec * 1000000);
#endif
#elif defined(BOOST_THREAD_CHRONO_MAC_API)
      kern_return_t err;
      threads::chrono_details::FP fp = threads::chrono_details::init_steady_clock(err);
      if ( err != 0  )
      {
        BOOST_ASSERT(0 && "Boost::Chrono - Internal Error");
      }
      return mono_platform_timepoint(fp());
#else
      timespec ts;
      if ( ::clock_gettime( CLOCK_MONOTONIC, &ts ) )
      {
        BOOST_ASSERT(0 && "Boost::Thread - clock_gettime(CLOCK_MONOTONIC) Internal Error");
        return mono_platform_timepoint(0);
      }
      return mono_platform_timepoint(ts);
#endif
    }
  };

#endif

#if defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
  typedef mono_platform_clock     internal_platform_clock;
  typedef mono_platform_timepoint internal_platform_timepoint;
#else
  typedef real_platform_clock     internal_platform_clock;
  typedef real_platform_timepoint internal_platform_timepoint;
#endif

#ifdef BOOST_THREAD_USES_CHRONO
#ifdef BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
  typedef chrono::steady_clock internal_chrono_clock;
#else
  typedef chrono::system_clock internal_chrono_clock;
#endif
#endif

  }
}

#include <boost/config/abi_suffix.hpp>

#endif
