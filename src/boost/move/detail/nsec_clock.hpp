//  This code is based on Timer and Chrono code. Thanks to authors:
//
//  Boost.Timer:
//  Copyright Beman Dawes 1994-2007, 2011
//
//  Boost.Chrono:
//  Copyright Beman Dawes 2008
//  Copyright 2009-2010 Vicente J. Botet Escriba
//
//  Simplified and modified to be able to support exceptionless (-fno-exceptions).
//  Boost.Timer depends on Boost.Chorno wich uses boost::throw_exception.
//  And Boost.Chrono DLLs don't build in Win32 as there is no 
//  boost::throw_exception(std::exception const&) implementation
//  in Boost.Chrono:
//
//  Copyright 2020 Ion Gaztanaga
//
//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//----------------------------------------------------------------------------//
//                                Windows                                     //
//----------------------------------------------------------------------------//
#ifndef BOOST_MOVE_DETAIL_NSEC_CLOCK_HPP
#define BOOST_MOVE_DETAIL_NSEC_CLOCK_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <cstdlib>


#   if (defined(_WIN32) || defined(__WIN32__) || defined(WIN32))
#     define BOOST_MOVE_DETAIL_WINDOWS_API
#   elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
#     define BOOST_MOVE_DETAIL_MAC_API
#   else
#     define BOOST_MOVE_DETAIL_POSIX_API
#   endif

#if defined(BOOST_MOVE_DETAIL_WINDOWS_API)

#include <boost/winapi/time.hpp>
#include <boost/winapi/timers.hpp>
#include <boost/winapi/get_last_error.hpp>
#include <boost/winapi/error_codes.hpp>
#include <boost/assert.hpp>
#include <boost/core/ignore_unused.hpp>

namespace boost { namespace move_detail {

template<int Dummy>
struct QPFHolder
{
   static inline double get_nsec_per_tic()
   {
      boost::winapi::LARGE_INTEGER_ freq;
      boost::winapi::BOOL_ r = boost::winapi::QueryPerformanceFrequency( &freq );
      boost::ignore_unused(r);
      BOOST_ASSERT(r != 0 && "Boost::Move - get_nanosecs_per_tic Internal Error");

      return double(1000000000.0L / freq.QuadPart);
   }

   static const double nanosecs_per_tic;
};

template<int Dummy>
const double QPFHolder<Dummy>::nanosecs_per_tic = get_nsec_per_tic();

inline boost::uint64_t nsec_clock() BOOST_NOEXCEPT
{
   double nanosecs_per_tic = QPFHolder<0>::nanosecs_per_tic;
   
   boost::winapi::LARGE_INTEGER_ pcount;
   unsigned times=0;
   while ( !boost::winapi::QueryPerformanceCounter( &pcount ) )
   {
      if ( ++times > 3 )
      {
         BOOST_ASSERT("Boost::Move - QueryPerformanceCounter Internal Error");
         return 0u;
      }
   }

   return static_cast<boost::uint64_t>((nanosecs_per_tic) * pcount.QuadPart);
}

}}  //namespace boost { namespace move_detail {

#elif defined(BOOST_MOVE_DETAIL_MAC_API)

#include <mach/mach_time.h>  // mach_absolute_time, mach_timebase_info_data_t

inline boost::uint64_t nsec_clock() BOOST_NOEXCEPT
{
   boost::uint64_t count = ::mach_absolute_time();

   mach_timebase_info_data_t info;
   mach_timebase_info(&info);
   return static_cast<boost::uint64_t>
      ( static_cast<double>(count)*(static_cast<double>(info.numer) / info.denom) );
}

#elif defined(BOOST_MOVE_DETAIL_POSIX_API)

#include <time.h>

#  if defined(CLOCK_MONOTONIC_PRECISE)   //BSD
#     define BOOST_MOVE_DETAIL_CLOCK_MONOTONIC CLOCK_MONOTONIC_PRECISE
#  elif defined(CLOCK_MONOTONIC_RAW)     //Linux
#     define BOOST_MOVE_DETAIL_CLOCK_MONOTONIC CLOCK_MONOTONIC_RAW
#  elif defined(CLOCK_HIGHRES)           //Solaris
#     define BOOST_MOVE_DETAIL_CLOCK_MONOTONIC CLOCK_HIGHRES
#  elif defined(CLOCK_MONOTONIC)         //POSIX (AIX, BSD, Linux, Solaris)
#     define BOOST_MOVE_DETAIL_CLOCK_MONOTONIC CLOCK_MONOTONIC
#  else
#     error "No high resolution steady clock in your system, please provide a patch"
#  endif

inline boost::uint64_t nsec_clock() BOOST_NOEXCEPT
{
   struct timespec count;
   ::clock_gettime(BOOST_MOVE_DETAIL_CLOCK_MONOTONIC, &count);
   boost::uint64_t r = count.tv_sec;
   r *= 1000000000U;
   r += count.tv_nsec;
   return r;
}

#endif  // POSIX

namespace boost { namespace move_detail {

typedef boost::uint64_t nanosecond_type;

struct cpu_times
{
   nanosecond_type wall;
   nanosecond_type user;
   nanosecond_type system;

   void clear() { wall = user = system = 0; }
};


inline void get_cpu_times(boost::move_detail::cpu_times& current)
{
    current.wall = nsec_clock();
}


class cpu_timer
{
   public:

      //  constructor
      cpu_timer() BOOST_NOEXCEPT                                   { start(); }

      //  observers
      bool          is_stopped() const BOOST_NOEXCEPT              { return m_is_stopped; }
      cpu_times     elapsed() const BOOST_NOEXCEPT;  // does not stop()

      //  actions
      void          start() BOOST_NOEXCEPT;
      void          stop() BOOST_NOEXCEPT;
      void          resume() BOOST_NOEXCEPT; 

   private:
      cpu_times     m_times;
      bool          m_is_stopped;
};


//  cpu_timer  ---------------------------------------------------------------------//

inline void cpu_timer::start() BOOST_NOEXCEPT
{
   m_is_stopped = false;
   get_cpu_times(m_times);
}

inline void cpu_timer::stop() BOOST_NOEXCEPT
{
   if (is_stopped())
      return;
   m_is_stopped = true;
      
   cpu_times current;
   get_cpu_times(current);
   m_times.wall = (current.wall - m_times.wall);
   m_times.user = (current.user - m_times.user);
   m_times.system = (current.system - m_times.system);
}

inline cpu_times cpu_timer::elapsed() const BOOST_NOEXCEPT
{
   if (is_stopped())
      return m_times;
   cpu_times current;
   get_cpu_times(current);
   current.wall -= m_times.wall;
   current.user -= m_times.user;
   current.system -= m_times.system;
   return current;
}

inline void cpu_timer::resume() BOOST_NOEXCEPT
{
   if (is_stopped())
   {
      cpu_times current (m_times);
      start();
      m_times.wall   -= current.wall;
      m_times.user   -= current.user;
      m_times.system -= current.system;
   }
}



}  // namespace move_detail
}  // namespace boost

#endif   //BOOST_MOVE_DETAIL_NSEC_CLOCK_HPP
