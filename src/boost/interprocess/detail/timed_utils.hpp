//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2021-2021.
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_DETAIL_TIMED_UTILS_HPP
#define BOOST_INTERPROCESS_DETAIL_TIMED_UTILS_HPP

#ifndef BOOST_CONFIG_HPP
#  include <boost/config.hpp>
#endif
#
#if defined(BOOST_HAS_PRAGMA_ONCE)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/intrusive/detail/mpl.hpp>

#include <ctime>
#include <boost/cstdint.hpp>

//The following is used to support high precision time clocks
#ifdef BOOST_HAS_GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef BOOST_HAS_FTIME
#include <time.h>
#include <boost/winapi/time.hpp>
#endif

namespace boost {
namespace interprocess {
namespace ipcdetail {

BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(time_duration_type)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(clock)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(rep_type)
BOOST_INTRUSIVE_INSTANTIATE_DEFAULT_TYPE_TMPLT(rep)

template<class T>
struct enable_if_ptime
   : enable_if_c< BOOST_INTRUSIVE_HAS_TYPE(boost::interprocess::ipcdetail::, T, time_duration_type) >
{};

template<class T>
struct disable_if_ptime
   : enable_if_c< ! BOOST_INTRUSIVE_HAS_TYPE(boost::interprocess::ipcdetail::, T, time_duration_type) >
{};

template<class T>
struct enable_if_ptime_duration
   : enable_if_c< BOOST_INTRUSIVE_HAS_TYPE(boost::interprocess::ipcdetail::, T, rep_type) >
{};

template<class T>
struct enable_if_time_point
   : enable_if_c< BOOST_INTRUSIVE_HAS_TYPE(boost::interprocess::ipcdetail::, T, clock) >
{};

template<class T>
struct enable_if_duration
   : enable_if_c< BOOST_INTRUSIVE_HAS_TYPE(boost::interprocess::ipcdetail::, T, rep) >
{};

#if defined(BOOST_INTERPROCESS_HAS_REENTRANT_STD_FUNCTIONS)

   inline std::tm* interprocess_gmtime(const std::time_t* t, std::tm* result)
   {
      // gmtime_r() not in namespace std???
      #if defined(__VMS) && __INITIAL_POINTER_SIZE == 64
         std::tm tmp;
         if(!gmtime_r(t,&tmp))
            result = 0;
         else
            *result = tmp;
      #else
         result = gmtime_r(t, result);
      #endif
      return result;
   }

#else // BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS

   #if defined(__clang__) // Clang has to be checked before MSVC
   #  pragma clang diagnostic push
   #  pragma clang diagnostic ignored "-Wdeprecated-declarations"
   #elif (defined(_MSC_VER) && (_MSC_VER >= 1400))
   #  pragma warning(push) // preserve warning settings
   #  pragma warning(disable : 4996) // disable depricated localtime/gmtime warning on vc8
   #endif

   inline std::tm* interprocess_gmtime(const std::time_t* t, std::tm* result)
   {
      result = std::gmtime(t);
      return result;
   }

   #if defined(__clang__) // Clang has to be checked before MSVC
   #  pragma clang diagnostic pop
   #elif (defined(_MSC_VER) && (_MSC_VER >= 1400))
   #  pragma warning(pop) // restore warnings to previous state
   #endif

#endif // BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS

#if defined(BOOST_HAS_FTIME)
/*!
* The function converts file_time into number of microseconds elapsed since 1970-Jan-01
*
* \note Only dates after 1970-Jan-01 are supported. Dates before will be wrapped.
*/
inline boost::uint64_t file_time_to_microseconds(const boost::winapi::FILETIME_ & ft)
{
   // shift is difference between 1970-Jan-01 & 1601-Jan-01
   // in 100-nanosecond units
   const boost::uint64_t shift = 116444736000000000ULL; // (27111902 << 32) + 3577643008

   // 100-nanos since 1601-Jan-01
   boost::uint64_t ft_as_integer = (static_cast< boost::uint64_t >(ft.dwHighDateTime) << 32) | static_cast< boost::uint64_t >(ft.dwLowDateTime);

   ft_as_integer -= shift; // filetime is now 100-nanos since 1970-Jan-01
   return (ft_as_integer / 10U); // truncate to microseconds
}
#endif

class ustime;

class usduration
{
   public:
   friend class ustime;

   explicit usduration(boost::uint64_t microsecs)
      : m_microsecs(microsecs)
   {}

   boost::uint64_t get_microsecs() const
   {  return m_microsecs;  }

   private:
   boost::uint64_t m_microsecs;
};

class ustime
{
   public:
   explicit ustime(boost::uint64_t microsecs)
      : m_microsecs(microsecs)
   {}

   ustime &operator += (const usduration &other)
   {  m_microsecs += other.m_microsecs; return *this; }

   ustime operator + (const usduration &other)
   {  ustime r(*this); r += other; return r; }

   bool operator < (const ustime &other) const
   {  return m_microsecs < other.m_microsecs; }

   bool operator > (const ustime &other) const
   {  return m_microsecs > other.m_microsecs; }

   bool operator <= (const ustime &other) const
   {  return m_microsecs <= other.m_microsecs; }

   bool operator >= (const ustime &other) const
   {  return m_microsecs >= other.m_microsecs; }

   boost::uint64_t get_microsecs() const
   {  return m_microsecs;  }

   private:
   boost::uint64_t m_microsecs;
};

inline usduration usduration_milliseconds(boost::uint64_t millisec)
{  return usduration(millisec*1000u);   }

template<class TimeType, class Enable = void>
class microsec_clock;

template<class TimeType>
class microsec_clock<TimeType, typename enable_if_ptime<TimeType>::type>
{
   private:
   typedef typename TimeType::date_type date_type;
   typedef typename TimeType::time_duration_type time_duration_type;
   typedef typename time_duration_type::rep_type resolution_traits_type;
   public:

   static TimeType universal_time()
   {
      #ifdef BOOST_HAS_GETTIMEOFDAY
         timeval tv;
         gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.
         std::time_t t = tv.tv_sec;
         boost::uint32_t sub_sec = tv.tv_usec;
      #elif defined(BOOST_HAS_FTIME)
         boost::winapi::FILETIME_ ft;
         boost::winapi::GetSystemTimeAsFileTime(&ft);
         boost::uint64_t micros = file_time_to_microseconds(ft); // it will not wrap, since ft is the current time
                                                                  // and cannot be before 1970-Jan-01
         std::time_t t = static_cast<std::time_t>(micros / 1000000UL); // seconds since epoch
         // microseconds -- static casts suppress warnings
         boost::uint32_t sub_sec = static_cast<boost::uint32_t>(micros % 1000000UL);
      #else
         #error "Unsupported date-time error: neither gettimeofday nor FILETIME support is detected"
      #endif

      std::tm curr;
      std::tm* curr_ptr = interprocess_gmtime(&t, &curr);
      date_type d(static_cast< typename date_type::year_type::value_type >(curr_ptr->tm_year + 1900),
                  static_cast< typename date_type::month_type::value_type >(curr_ptr->tm_mon + 1),
                  static_cast< typename date_type::day_type::value_type >(curr_ptr->tm_mday));

      //The following line will adjust the fractional second tick in terms
      //of the current time system.  For example, if the time system
      //doesn't support fractional seconds then res_adjust returns 0
      //and all the fractional seconds return 0.
      int adjust = static_cast< int >(resolution_traits_type::res_adjust() / 1000000);

      time_duration_type td(static_cast< typename time_duration_type::hour_type >(curr_ptr->tm_hour),
                              static_cast< typename time_duration_type::min_type >(curr_ptr->tm_min),
                              static_cast< typename time_duration_type::sec_type >(curr_ptr->tm_sec),
                              sub_sec * adjust);

      return TimeType(d,td);
   }
};

template<>
class microsec_clock<ustime>
{
   public:
   static ustime universal_time()
   {
      #ifdef BOOST_HAS_GETTIMEOFDAY
         timeval tv;
         gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.
         boost::uint64_t micros = boost::uint64_t(tv.tv_sec)*1000000;
         micros += tv.tv_usec;
      #elif defined(BOOST_HAS_FTIME)
         boost::winapi::FILETIME_ ft;
         boost::winapi::GetSystemTimeAsFileTime(&ft);
         boost::uint64_t micros = file_time_to_microseconds(ft); // it will not wrap, since ft is the current time
                                                                  // and cannot be before 1970-Jan-01
      #else
         #error "Unsupported date-time error: neither gettimeofday nor FILETIME support is detected"
      #endif
      return ustime(micros);
   }
};

template<class TimePoint>
class microsec_clock<TimePoint, typename enable_if_time_point<TimePoint>::type>
{
   public:
   static TimePoint universal_time()
   {  return TimePoint::clock::now();  }
};


template<class TimePoint>
inline TimePoint delay_ms(unsigned msecs, typename enable_if_ptime<TimePoint>::type* = 0)
{
   typedef typename TimePoint::time_duration_type time_duration_type;
   typedef typename time_duration_type::rep_type resolution_traits_type;

   time_duration_type td(msecs*1000*resolution_traits_type::res_adjust());

   TimePoint tp(microsec_clock<TimePoint>::universal_time());
   return (tp += td);
}

template<class TimePoint>
inline bool is_pos_infinity(const TimePoint &abs_time, typename enable_if_ptime<TimePoint>::type* = 0)
{
   return abs_time.is_pos_infinity();
}

template<class TimePoint>
inline bool is_pos_infinity(const TimePoint &, typename disable_if_ptime<TimePoint>::type* = 0)
{
   return false;
}

template<class Duration>
inline boost::uint64_t duration_to_milliseconds(const Duration &abs_time, typename enable_if_ptime_duration<Duration>::type* = 0)
{
   return abs_time.total_milliseconds();
}

template<class Duration>
inline boost::uint64_t duration_to_milliseconds(const Duration &d, typename enable_if_duration<Duration>::type* = 0)
{
   const double factor = double(Duration::period::num)*1000.0/double(Duration::period::den);
   return static_cast<boost::uint64_t>(d.count()*factor);
}


}  //namespace ipcdetail {
}  //namespace interprocess {
}  //namespace boost {

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_DETAIL_TIMED_UTILS_HPP

