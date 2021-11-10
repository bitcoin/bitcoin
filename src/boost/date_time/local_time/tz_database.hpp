#ifndef BOOST_DATE_TIME_TZ_DATABASE_HPP__
#define BOOST_DATE_TIME_TZ_DATABASE_HPP__

/* Copyright (c) 2003-2004 CrystalClear Software, Inc.
 * Subject to the Boost Software License, Version 1.0. 
 * (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)
 * Author: Jeff Garland, Bart Garst
 * $Date$
 */

#include <string>
#include "boost/date_time/local_time/custom_time_zone.hpp"
#include "boost/date_time/local_time/dst_transition_day_rules.hpp"
#include "boost/date_time/tz_db_base.hpp"


namespace boost {
namespace local_time {

  using date_time::data_not_accessible; 
  using date_time::bad_field_count; 

  //! Object populated with boost::shared_ptr<time_zone_base> objects
  /*! Object populated with boost::shared_ptr<time_zone_base> objects
   * Database is populated from specs stored in external csv file. See
   * date_time::tz_db_base for greater detail */
  typedef date_time::tz_db_base<custom_time_zone, nth_kday_dst_rule> tz_database;

}} // namespace

#endif // BOOST_DATE_TIME_TZ_DATABASE_HPP__

