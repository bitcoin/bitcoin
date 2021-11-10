#ifndef POSIX_TIME_CONVERSION_HPP___
#define POSIX_TIME_CONVERSION_HPP___

/* Copyright (c) 2002-2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <cstring>
#include <boost/cstdint.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/filetime_functions.hpp>
#include <boost/date_time/c_time.hpp>
#include <boost/date_time/time_resolution_traits.hpp> // absolute_value
#include <boost/date_time/gregorian/conversion.hpp>

namespace boost {

namespace posix_time {

  //! Function that converts a time_t into a ptime.
  inline
  ptime from_time_t(std::time_t t)
  {
    return ptime(gregorian::date(1970,1,1)) + seconds(static_cast<long>(t));
  }

  //! Function that converts a ptime into a time_t
  inline
  std::time_t to_time_t(ptime pt)
  {
    return (pt - ptime(gregorian::date(1970,1,1))).total_seconds();
  }

  //! Convert a time to a tm structure truncating any fractional seconds
  inline
  std::tm to_tm(const boost::posix_time::ptime& t) {
    std::tm timetm = boost::gregorian::to_tm(t.date());
    boost::posix_time::time_duration td = t.time_of_day();
    timetm.tm_hour = static_cast<int>(td.hours());
    timetm.tm_min = static_cast<int>(td.minutes());
    timetm.tm_sec = static_cast<int>(td.seconds());
    timetm.tm_isdst = -1; // -1 used when dst info is unknown
    return timetm;
  }
  //! Convert a time_duration to a tm structure truncating any fractional seconds and zeroing fields for date components
  inline
  std::tm to_tm(const boost::posix_time::time_duration& td) {
    std::tm timetm;
    std::memset(&timetm, 0, sizeof(timetm));
    timetm.tm_hour = static_cast<int>(date_time::absolute_value(td.hours()));
    timetm.tm_min = static_cast<int>(date_time::absolute_value(td.minutes()));
    timetm.tm_sec = static_cast<int>(date_time::absolute_value(td.seconds()));
    timetm.tm_isdst = -1; // -1 used when dst info is unknown
    return timetm;
  }

  //! Convert a tm struct to a ptime ignoring is_dst flag
  inline
  ptime ptime_from_tm(const std::tm& timetm) {
    boost::gregorian::date d = boost::gregorian::date_from_tm(timetm);
    return ptime(d, time_duration(timetm.tm_hour, timetm.tm_min, timetm.tm_sec));
  }


#if defined(BOOST_HAS_FTIME)

  //! Function to create a time object from an initialized FILETIME struct.
  /*! Function to create a time object from an initialized FILETIME struct.
   * A FILETIME struct holds 100-nanosecond units (0.0000001). When
   * built with microsecond resolution the FILETIME's sub second value
   * will be truncated. Nanosecond resolution has no truncation.
   *
   * \note FILETIME is part of the Win32 API, so it is not portable to non-windows
   * platforms.
   *
   * \note The function is templated on the FILETIME type, so that
   *       it can be used with both native FILETIME and the ad-hoc
   *       boost::detail::winapi::FILETIME_ type.
   */
  template< typename TimeT, typename FileTimeT >
  inline
  TimeT from_ftime(const FileTimeT& ft)
  {
    return boost::date_time::time_from_ftime<TimeT>(ft);
  }

#endif // BOOST_HAS_FTIME

} } //namespace boost::posix_time




#endif

