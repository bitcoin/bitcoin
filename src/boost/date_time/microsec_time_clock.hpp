#ifndef DATE_TIME_HIGHRES_TIME_CLOCK_HPP___
#define DATE_TIME_HIGHRES_TIME_CLOCK_HPP___

/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */


/*! @file microsec_time_clock.hpp
  This file contains a high resolution time clock implementation.
*/

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/c_time.hpp>
#include <boost/date_time/time_clock.hpp>
#if defined(BOOST_HAS_FTIME)
#include <boost/winapi/time.hpp>
#endif

#ifdef BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK

namespace boost {
namespace date_time {

  //! A clock providing microsecond level resolution
  /*! A high precision clock that measures the local time
   *  at a resolution up to microseconds and adjusts to the
   *  resolution of the time system.  For example, for the
   *  a library configuration with nano second resolution,
   *  the last 3 places of the fractional seconds will always
   *  be 000 since there are 1000 nano-seconds in a micro second.
   */
  template<class time_type>
  class microsec_clock
  {
  private:
    //! Type for the function used to convert time_t to tm
    typedef std::tm* (*time_converter)(const std::time_t*, std::tm*);

  public:
    typedef typename time_type::date_type date_type;
    typedef typename time_type::time_duration_type time_duration_type;
    typedef typename time_duration_type::rep_type resolution_traits_type;

    //! return a local time object for the given zone, based on computer clock
    //JKG -- looks like we could rewrite this against universal_time
    template<class time_zone_type>
    static time_type local_time(shared_ptr<time_zone_type> tz_ptr)
    {
      typedef typename time_type::utc_time_type utc_time_type;
      typedef second_clock<utc_time_type> second_clock;
      // we'll need to know the utc_offset this machine has
      // in order to get a utc_time_type set to utc
      utc_time_type utc_time = second_clock::universal_time();
      time_duration_type utc_offset = second_clock::local_time() - utc_time;
      // use micro clock to get a local time with sub seconds
      // and adjust it to get a true utc time reading with sub seconds
      utc_time = microsec_clock<utc_time_type>::local_time() - utc_offset;
      return time_type(utc_time, tz_ptr);
    }

    //! Returns the local time based on computer clock settings
    static time_type local_time()
    {
      return create_time(&c_time::localtime);
    }

    //! Returns the UTC time based on computer settings
    static time_type universal_time()
    {
      return create_time(&c_time::gmtime);
    }

  private:
    static time_type create_time(time_converter converter)
    {
#ifdef BOOST_HAS_GETTIMEOFDAY
      timeval tv;
      gettimeofday(&tv, 0); //gettimeofday does not support TZ adjust on Linux.
      std::time_t t = tv.tv_sec;
      boost::uint32_t sub_sec = tv.tv_usec;
#elif defined(BOOST_HAS_FTIME)
      boost::winapi::FILETIME_ ft;
      boost::winapi::GetSystemTimeAsFileTime(&ft);
#if BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3205))
      // Some runtime library implementations expect local times as the norm for ctime functions.
      {
        boost::winapi::FILETIME_ local_ft;
        boost::winapi::FileTimeToLocalFileTime(&ft, &local_ft);
        ft = local_ft;
      }
#endif

      boost::uint64_t micros = file_time_to_microseconds(ft); // it will not wrap, since ft is the current time
                                                              // and cannot be before 1970-Jan-01
      std::time_t t = static_cast<std::time_t>(micros / 1000000UL); // seconds since epoch
      // microseconds -- static casts suppress warnings
      boost::uint32_t sub_sec = static_cast<boost::uint32_t>(micros % 1000000UL);
#else
#error Internal Boost.DateTime error: BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK is defined, however neither gettimeofday nor FILETIME support is detected.
#endif

      std::tm curr;
      std::tm* curr_ptr = converter(&t, &curr);
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

      return time_type(d,td);
    }

#if defined(BOOST_HAS_FTIME)
    /*!
     * The function converts file_time into number of microseconds elapsed since 1970-Jan-01
     *
     * \note Only dates after 1970-Jan-01 are supported. Dates before will be wrapped.
     */
    static boost::uint64_t file_time_to_microseconds(boost::winapi::FILETIME_ const& ft)
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
  };


} } //namespace date_time

#endif //BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK


#endif

