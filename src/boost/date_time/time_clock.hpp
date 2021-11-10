#ifndef DATE_TIME_TIME_CLOCK_HPP___
#define DATE_TIME_TIME_CLOCK_HPP___

/* Copyright (c) 2002,2003,2005 CrystalClear Software, Inc.
 * Use, modification and distribution is subject to the
 * Boost Software License, Version 1.0. (See accompanying
 * file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

/*! @file time_clock.hpp
  This file contains the interface for clock devices.
*/

#include "boost/date_time/c_time.hpp"
#include "boost/shared_ptr.hpp"

namespace boost {
namespace date_time {


  //! A clock providing time level services based on C time_t capabilities
  /*! This clock provides resolution to the 1 second level
   */
  template<class time_type>
  class second_clock
  {
  public:
    typedef typename time_type::date_type date_type;
    typedef typename time_type::time_duration_type time_duration_type;

    static time_type local_time()
    {
      ::std::time_t t;
      ::std::time(&t);
      ::std::tm curr, *curr_ptr;
      //curr_ptr = ::std::localtime(&t);
      curr_ptr = c_time::localtime(&t, &curr);
      return create_time(curr_ptr);
    }


    //! Get the current day in universal date as a ymd_type
    static time_type universal_time()
    {

      ::std::time_t t;
      ::std::time(&t);
      ::std::tm curr, *curr_ptr;
      //curr_ptr = ::std::gmtime(&t);
      curr_ptr = c_time::gmtime(&t, &curr);
      return create_time(curr_ptr);
    }

    template<class time_zone_type>
    static time_type local_time(boost::shared_ptr<time_zone_type> tz_ptr)
    {
      typedef typename time_type::utc_time_type utc_time_type;
      utc_time_type utc_time = second_clock<utc_time_type>::universal_time();
      return time_type(utc_time, tz_ptr);
    }


  private:
    static time_type create_time(::std::tm* current)
    {
      date_type d(static_cast<unsigned short>(current->tm_year + 1900),
                  static_cast<unsigned short>(current->tm_mon + 1),
                  static_cast<unsigned short>(current->tm_mday));
      time_duration_type td(current->tm_hour,
                            current->tm_min,
                            current->tm_sec);
      return time_type(d,td);
    }

  };


} } //namespace date_time


#endif
