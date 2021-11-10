//  (C) Copyright Raffi Enficiaud 2019.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  Description : timer and elapsed types
// ***************************************************************************

#ifndef BOOST_TEST_UTILS_TIMER_HPP
#define BOOST_TEST_UTILS_TIMER_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <utility>
#include <ctime>

# if defined(_WIN32) || defined(__CYGWIN__)
#   define BOOST_TEST_TIMER_WINDOWS_API
# elif defined(__MACH__) && defined(__APPLE__)// && !defined(CLOCK_MONOTONIC)
#   // we compile for all macs the same, CLOCK_MONOTONIC introduced in 10.12
#   define BOOST_TEST_TIMER_MACH_API
# else
#   define BOOST_TEST_TIMER_POSIX_API
#   if !defined(CLOCK_MONOTONIC)
#     error "CLOCK_MONOTONIC not defined"
#   endif
# endif

# if defined(BOOST_TEST_TIMER_WINDOWS_API)
#   include <windows.h>
# elif defined(BOOST_TEST_TIMER_MACH_API)
#   include <mach/mach_time.h>
//#   include <mach/mach.h>      /* host_get_clock_service, mach_... */
# else
#   include <sys/time.h>
# endif

# ifdef BOOST_NO_STDC_NAMESPACE
  namespace std { using ::clock_t; using ::clock; }
# endif

namespace boost {
namespace unit_test {
namespace timer {

  struct elapsed_time
  {
    typedef boost::int_least64_t nanosecond_type;

    nanosecond_type wall;
    nanosecond_type system;
    void clear() {
      wall = 0;
      system = 0;
    }
  };

  inline double
  microsecond_wall_time( elapsed_time const& elapsed )
  {
      return elapsed.wall / 1E3;
  }

  inline double
  second_wall_time( elapsed_time const& elapsed )
  {
      return elapsed.wall / 1E9;
  }

  namespace details {
    #if defined(BOOST_TEST_TIMER_WINDOWS_API)
    elapsed_time::nanosecond_type get_tick_freq() {
        LARGE_INTEGER freq;
        ::QueryPerformanceFrequency( &freq );
        return static_cast<elapsed_time::nanosecond_type>(freq.QuadPart);
    }
    #elif defined(BOOST_TEST_TIMER_MACH_API)
    std::pair<elapsed_time::nanosecond_type, elapsed_time::nanosecond_type> get_time_base() {
        mach_timebase_info_data_t timebase;
        if(mach_timebase_info(&timebase) == 0)
            return std::pair<elapsed_time::nanosecond_type, elapsed_time::nanosecond_type>(timebase.numer, timebase.denom);
        return std::pair<elapsed_time::nanosecond_type, elapsed_time::nanosecond_type>(0, 1);
    }
    #endif
  }

  //! Simple timing class
  //!
  //! This class measures the wall clock time.
  class timer
  {
  public:
    timer()
    {
        restart();
    }
    void restart()
    {
        _start_time_clock = std::clock();
    #if defined(BOOST_TEST_TIMER_WINDOWS_API)
        ::QueryPerformanceCounter(&_start_time_wall);
    #elif defined(BOOST_TEST_TIMER_MACH_API)
        _start_time_wall = mach_absolute_time();
    #else
        if( ::clock_gettime( CLOCK_MONOTONIC, &_start_time_wall ) != 0 )
        {
            _start_time_wall.tv_nsec = -1;
            _start_time_wall.tv_sec = -1;
        }
    #endif
    }

    // return elapsed time in seconds
    elapsed_time elapsed() const
    {
      typedef elapsed_time::nanosecond_type nanosecond_type;
      static const double clock_to_nano_seconds = 1E9 / CLOCKS_PER_SEC;
      elapsed_time return_value;

      // processor / system time
      return_value.system = static_cast<nanosecond_type>(double(std::clock() - _start_time_clock) * clock_to_nano_seconds);

#if defined(BOOST_TEST_TIMER_WINDOWS_API)
      static const nanosecond_type tick_per_sec = details::get_tick_freq();
      LARGE_INTEGER end_time;
      ::QueryPerformanceCounter(&end_time);
      return_value.wall = static_cast<nanosecond_type>(((end_time.QuadPart - _start_time_wall.QuadPart) * 1E9) / tick_per_sec);
#elif defined(BOOST_TEST_TIMER_MACH_API)
      static std::pair<nanosecond_type, nanosecond_type> timebase = details::get_time_base();
      nanosecond_type clock = mach_absolute_time() - _start_time_wall;
      return_value.wall = static_cast<nanosecond_type>((clock * timebase.first) / timebase.second);
#else
      struct timespec end_time;
      return_value.wall = 0;
      if( ::clock_gettime( CLOCK_MONOTONIC, &end_time ) == 0 )
      {
          return_value.wall = static_cast<nanosecond_type>((end_time.tv_sec - _start_time_wall.tv_sec) * 1E9 + (end_time.tv_nsec - _start_time_wall.tv_nsec));
      }
#endif

      return return_value;
    }

   private:
      std::clock_t _start_time_clock;
    #if defined(BOOST_TEST_TIMER_WINDOWS_API)
      LARGE_INTEGER _start_time_wall;
    #elif defined(BOOST_TEST_TIMER_MACH_API)
      elapsed_time::nanosecond_type _start_time_wall;
    #else
      struct timespec _start_time_wall;
    #endif
  };


//____________________________________________________________________________//

} // namespace timer
} // namespace unit_test
} // namespace boost

#endif // BOOST_TEST_UTILS_TIMER_HPP

